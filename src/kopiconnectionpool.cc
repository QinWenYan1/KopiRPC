#include "KopiConnectpool.h"
#include <netinet/tcp.h>

// 假设每个 IP:Port 最多简历 1024 个长链接 （必须是 2 的幂）
static const int MAX_CONN_SIZE = 1024; 

KopiConnectPool &KopiConnectPool::GetInstance(){
    static KopiConnectPool instance; 
    return instance; 
}

