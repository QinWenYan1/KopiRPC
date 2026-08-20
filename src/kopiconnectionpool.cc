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

KopiConnectPool::~KopiConnectPool(){
    std::lock_guard<std::mutex> lock(globalMtx); 
    for ( auto &pair : pools ){
        int fd; 
        //这里为什么使用while
        while ((fd = pair.second->freeFds.front())){
            close(fd); 
        }
        delete pair.second ;
    }
}