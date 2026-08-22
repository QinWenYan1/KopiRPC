#pragma once
#include <cstddef>  // size_t

//把 data[0..len) 全部写进 fd: 循环处理部分写入,EINTR 重试;
//MSG_NOSIGNAL 防对端已关时 SIGPIPE 杀进程(连接池场景必带)
bool SendAll(int fd, const char* data, size_t len);

//从 fd 读满 len 字节到 buf: 读满 true;对端关闭(recv=0)或出错 false
bool RecvN(int fd, char* buf, size_t len);