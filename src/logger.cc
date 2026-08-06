#include "logger.h"

//获取日志的单例
Logger& Logger::GetInstance() {
  static Logger logger;
  return logger;
}

//启动专门的写日志线程
Logger::Logger() {
  std::thread writeLogTask([&]() {
    for (;;) {
      //获取当前的日期，然后取日志信息，写入相应的日志文件当中
    }
  });
  //设置分离线程，守护线程
  writeLogTask.detach();
}

//设置日志级别
void Logger::SetLogLevel(LogLevel level) {}

//写日志，把日志信息写到lockqueue缓冲区当中
void Logger::Log(std::string msg) { logQueue.Push(msg); }