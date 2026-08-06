#pragma once
#include <string>

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
  int loglevel;                     //记录日志级别
  LockQueue<std::string> logQueue;  //日志缓冲队列

  Logger();
  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
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