#include "KopiConnectpool.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <mutex>
#include <string>

// 假设每个 IP:Port 最多简历 1024 个长链接 （必须是 2 的幂）
static const int MAX_CONN_SIZE = 1024; 

KopiConnectPool &KopiConnectPool::GetInstance(){
    static KopiConnectPool instance; 
    return instance; 
}

KopiConnectPool::KopiConnectPool(): maxConnPerNode(MAX_CONN_SIZE){}

// 逐一销毁桶中的free fds 
KopiConnectPool::~KopiConnectPool(){
    std::lock_guard<std::mutex> lock(globalMtx); 
    for ( auto &pair : pools ){
        int fd; 
        //这里为什么使用while
        auto q = pair.second->freeFds; 
        while (q.empty()){
            fd = pair.second->freeFds.front();
            close(fd); 
            pair.second->freeFds.pop(); 
        }
        delete pair.second;
    }
}

// 借用链接：
    //      如果对应节点的 bucket 中有空闲链接则返回，否则新建；
    //      若达到上线则阻塞等待
int KopiConnectPool::BorrowConnection(const std::string& ip, uint16_t port){
    std::string key = ip + ":" + std::to_string(port); 
    ConnectionBucket* bucket = nullptr; 

    // 1.   
    // 查询全局 map 找到对应的 bucket
    // 值在查找和创建 bucket 时加全局锁，粒度及其小
    {
        std::lock_guard<std::mutex> lock(globalMtx); 
        if (pools.find(key) == pools.end()){
            //没有找到创建新的bucket
            pools[key] = new ConnectionBucket(); 
        }
        bucket = pools[key]; 
    }

    // 2. 对当前节点的bucket加锁
    std::unique_lock<std::mutex> lock(bucket->mtx);

    // 情况 A: 如果有现成空闲长链接，直接出队返回，性能最高
    if (!bucket->freeFds.empty()){
        int fd = bucket->freeFds.front(); 
        bucket->freeFds.pop(); 
        return fd; 
    }

    //情况 B: 没有空闲的链接，并且当前链接的链接数量还咩有达到 1024 上限，允许新建链接
    if (bucket->active_count < maxConnPerNode){
        bucket->active_count++; //预占位

        lock.unlock(); //关键设计：在进行耗时的 connect 系统调用前必须解锁，避免阻塞其他线程
        // IPv4地址，面向链接的字节流 socket ， 并让操作系统根据前面的参数选择合适的协议
        int fd = socket(AF_INET,SOCK_STREAM,0); 

        // 核心优化：禁止 TCP nagle 算法
        //          nagle 算法：“这个包这么小，要不要等一等，看看后面有没有更多数据，可以合并起来一起发？”
        //          减少网络开销，提高吞吐量，但可能增加延迟
        //          Rpc request 并不大，我们更中意 低延迟
        // 小数据包不等待，直接发出，延迟大幅度降低
        int opt = 1; 
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

        struct sockaddr_in addr; 
        addr.sin_family = AF_INET; 
        addr.sin_port = htons(port); 
        addr.sin_addr.s_addr = inet_addr(ip.c_str()); 

        // 发起同步阻塞 TCP 链接
        if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0){
            return fd; // 链接成功
        }else{
            close(fd); // 链接失败
            lock.lock(); 
            bucket->active_count.fetch_sub(1); // 计数自减
            bucket->cv.notify_one(); 
            return -1; 
        }

    }   
    // 情况 C: 链接已经达到上限（1024），线程阻塞挂起，等待其他线程 ReturnConnection 释放链接
    if (bucket->cv.wait_for(lock,std::chrono::milliseconds(1000), [&]{return !bucket->freeFds.empty();})){
        int fd = bucket->freeFds.front(); 
        bucket->freeFds.pop(); 
        return fd; //在 1000 毫秒内等到了可使用的链接
    }
    return -1; //等到超时，降级报错

}

// 归还链接
void KopiConnectPool::ReturnConnection(const std::string &ip, uint16_t port, int fd, bool isBad){
    std::string key = ip + ":" + std::to_string(port); 

    ConnectionBucket* bucket = nullptr; 
    {
        std::lock_guard<std::mutex> lock(globalMtx); 
        auto it = pools.find(key);
        if (it != pools.end()) bucket = it->second; 
    }

    if (!bucket){
        close(fd); 
        return; 
    }

    std::lock_guard<std::mutex> lock(bucket->mtx); 
    // 如果通信过程中发生过网络断开，对端关闭异常 (isTrue = true)
    if (isBad){
        close(fd);                     // 彻底关闭不可用的 socket 资源
        bucket->active_count.fetch_sub(1); // 活动链接数扣减，允许后续重新建立新链接
    }else{
        bucket->freeFds.push(fd);   //依旧健康的长链接，放回桶队列中的备用
    }
    bucket->cv.notify_one();           // 唤醒可能正在等待链接的 Borrow 线程
}