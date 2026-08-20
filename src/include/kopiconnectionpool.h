#pragma once 

#include <condition_variable>
#include <queue>
#include <mutex>
#include <atomic>

struct ConnectionBucket {
    std::queue<int> free_fds;       // 存放该服务节点空闲的socket fd 队列
    std::mutex mtx;                 // 保护当前桶内部 free_fds 队列的互斥锁
    std::condition_variable cv;     // 条件变量，当链接达到上限，线程等待空闲链接时使用
    std::atomic<int> active_count;  // 已经成为该节点创建的所有活动 TCP 链接总数

    ConnectionBucket() : active_count(0) {} 
}; 


class KopiConnectPool {
    public:
    // 获取链接池单例
    static KopiConnectPool &GetInstance(); 

    // 借用链接：
    //      如果对应节点的 bucket 中有空闲链接则返回，否则新建；
    //      若达到上线则阻塞等待
    int BorrowConnection(const std::string&, uint16_t); 

    // 归还链接：
    //      使用完毕后必须归还，如果链接已经损坏（is_bad = true）
    //      则直接关闭并扣减 active_count
    void ReturnConnection(const std::string&, uint16_t, int, bool); 

    private:
    KopiConnectPool(); 
    ~KopiConnectPool(); 
    
}; 

