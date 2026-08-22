// netutil —— TCP 收发小工具: SendN / RecvN
//
// 动机(解决什么痛点):
//   TCP 是字节流协议, send/recv 都"不包干":
//     ① send 的返回值才是实际发出的字节数, 可能 < len(部分写入)
//        —— 内核发送缓冲满了就只收编一部分, 剩下的没人替你发
//     ② recv 一次能读到多少不确定, 可能 < 你要的字节数
//        —— 流没有消息边界, "把 size 传给对方, 对方就会一次发全"是错觉
//   所以每个调用点本该都写同一个"没发完/没读够就再来一次"的循环;
//   本文件把循环收敛成两个函数, 调用点一行搞定:
//     SendN —— 循环发, 直到 len 字节全部发出才 true
//     RecvN   —— 循环收, 读满 len 字节才 true(配合长度头确定一条消息的边界)
//
// 关键细节:
//   · send 带 MSG_NOSIGNAL: 对端已关闭时默认触发 SIGPIPE 会杀掉整个进程,
//     带此 flag 改为返回 -1/EPIPE, 走正常错误路径(连接池复用旧连接场景必带)
//   · EINTR(系统调用被信号打断)不算错误, 直接重试
#pragma once
#include <cstddef>  // size_t

//把 data[0..len) 全部写进 fd: 循环处理部分写入,EINTR 重试;
//MSG_NOSIGNAL 防对端已关时 SIGPIPE 杀进程(连接池场景必带)
bool SendN(int fd, const char* data, size_t len);

//从 fd 读满 len 字节到 buf: 读满 true;对端关闭(recv=0)或出错 false
bool SafeN(int fd, char* buf, size_t len);