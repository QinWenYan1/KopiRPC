#pragma once
#include <atomic>  //新增
#include <string>
#include <thread>  //新增

#include "lockqueue.h"

enum LogLevel {
  INFO,   //普通信息
  ERROR,  //错误信息
};
// kopirpc框架提供的日志系统：使用单例模式
class Logger {
 public:
  //获取日志的单例
  static Logger& GetInstance();
  //设置日志级别
  void SetLogLevel(LogLevel level);
  //写日志
  void Log(std::string msg);

 private:
  int loglevel;                       //记录日志级别
  LockQueue<std::string> logQueue;    //日志缓冲队列
  std::atomic<bool> stopping{false};  //停机标志: Stop() 置位,写线程醒来自查
  std::thread writeThread;            //写日志线程(成员持有,Stop 时 join 回收)

  Logger();
  ~Logger();  //析构即停机: 进程退出时自动调 Stop(),使用方零感知
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;

  //停止写日志线程: 置标志 + 发哨兵唤醒 + join 回收(幂等,可重复调)
  //仅析构调用;private 防止外部在运行中途掐死日志
  void Stop();
};

// 定义日志宏给用户更方便的方式去日志写入，例如：LOG_INFO("xx %d %s", 20,
// "xxxx")
#define LOG_INFO(logmsgformat, ...)                 \
  do {                                              \
    Logger& logger = Logger::GetInstance();         \
    logger.SetLogLevel(INFO);                       \
    char c[1024] = {0};                             \
    snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
    logger.Log(c);                                  \
  } while (0)

  
#define LOG_ERR(logmsgformat, ...)                  \
  do {                                              \
    Logger& logger = Logger::GetInstance();         \
    logger.SetLogLevel(ERROR);                      \
    char c[1024] = {0};                             \
    snprintf(c, 1024, logmsgformat, ##__VA_ARGS__); \
    logger.Log(c);                                  \
  } while (0)
// ⚠️ 关键：请确保上面这行 while(0)
// 敲完回车后，下面还有一行空行（即文件以换行符结束）