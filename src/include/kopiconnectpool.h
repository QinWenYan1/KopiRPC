// KopiConnectPool —— RPC 客户端连接池(单例)
//
// 动机(解决什么痛点):
//   旧版 caller 每次 RPC 调用都新建一条 TCP 连接,用完即关(短连接):
//     ① 每次调用白付一次 TCP 三次握手/四次挥手,延迟大头全在连接管理上
//     ② 高并发下海量短连接产生大量 TIME_WAIT: 主动关闭方的源端口被锁定 60 秒,
//       而客户端临时端口总共只有 ~2.8 万个,耗尽后 connect 直接失败
//   本模块做法: 为每个服务节点(IP:Port)维护一个"连接桶",调用时借出空闲
//   长连接,用完归还复用 —— 连接只建一次,反复收发,TIME_WAIT 问题根治
//
// 使用契约:
//   BorrowConnection 借到 fd → 用完必须 ReturnConnection 归还;
//   通信中发生过任何 IO 错误 → 以 is_bad=true 归还(池直接焚毁,防止坏连接污染复用)
#pragma once 

#include <condition_variable>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <cstdint>

struct ConnectionBucket {
    std::queue<int> freeFds;       // 存放该服务节点空闲的socket fd 队列
    std::mutex mtx;                 // 保护当前桶内部 free_fds 队列的互斥锁
    std::condition_variable cv;     // 条件变量，当链接达到上限，线程等待空闲链接时使用
    std::atomic<int> active_count;  // 已经成为该节点创建的所有活动 TCP 链接总数

    ConnectionBucket(int nums = 0) : active_count(nums) {}
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

    int maxConnPerNode;                                         // 每个服务节点允许的最大 TCP 链接上限
    std::unordered_map<std::string, ConnectionBucket*> pools;   // 映射："IP:Port" -> 该节点的链接桶
    std::mutex globalMtx;                                       // 保护 pools 中 bucket 映射创建时的全局互斥锁

}; 

