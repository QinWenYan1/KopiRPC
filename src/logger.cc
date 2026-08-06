#include "logger.h"

#include <time.h>

#include <cstdlib>
#include <iostream>

//获取日志的单例
Logger& Logger::GetInstance() {
  static Logger logger;
  return logger;
}

//启动专门的写日志线程
Logger::Logger() {
  std::thread writeLogTask([&]() {
    for (;;) {
      //获取当前的日期，然后取日志信息，写入相应的日志文件当中，也就是说追加的方式
      time_t now = time(nullptr);
      tm* nowTime = localtime(&now);
      std::string fileName = std::to_string(nowTime->tm_year + 1900) + "-" +
                       std::to_string(nowTime->tm_mon + 1) + "-" +
                       std::to_string(nowTime->tm_mday) + "-log.txt";
      FILE* fp = fopen(fileName.c_str(), "a+");
      if (fp == nullptr) {
        std::cout << "logger file: " << fileName << " open error!" << std::endl;
        exit(EXIT_FAILURE);
      }

      std::string msg = logQueue.Pop();
      fputs(msg.c_str(), fp);
      fclose(fp); 
    }
  });
  //设置分离线程，守护线程
  writeLogTask.detach();
}

//设置日志级别
void Logger::SetLogLevel(LogLevel level) {}

//写日志，把日志信息写到lockqueue缓冲区当中
void Logger::Log(std::string msg) { logQueue.Push(msg); }