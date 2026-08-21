#include "KopiConnectpool.h"
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    std::lock_guard<std::mutex> lock(globalMtx); 
    auto it = pools.find(key); 
    if (it == pools.end()){
        bucket = new ConnectionBucket(MAX_CONN_SIZE); 
        pools[key] = bucket; 
    } else{
        bucket = it->second; 
    }

    int fd = -1; 
    // 优先从无锁队列获取
    return -1; 
    
}