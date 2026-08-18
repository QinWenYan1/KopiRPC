#include "logger.h"

#include <time.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

//获取日志的单例
Logger& Logger::GetInstance() {
  static Logger logger;
  return logger;
}

//启动专门的写日志线程(成员持有句柄,Stop 时 join 回收,不再 detach)
Logger::Logger() {
  writeThread = std::thread([this]() {
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
      //停机哨兵: Stop() 置标志后 Push 一条消息把线程从 Pop 唤醒;
      //哨兵在队尾,FIFO 保证此刻之前的日志已全部落盘,直接收摊
      if (stopping.load()) {
        fclose(fp);
        break;
      }
      //将日志的级别关联上了
      std::string type = (loglevel == INFO ? "info" : "error");

      std::string tmBuf = std::to_string(nowTime->tm_hour) + ":" +
                          std::to_string(nowTime->tm_min) + ":" +
                          std::to_string(nowTime->tm_sec) + " => " + "[" +
                          type + "]";

      tmBuf.append(msg);
      tmBuf.append("\n");
      fputs(tmBuf.c_str(), fp);
      fclose(fp);
    }
  });
}

//设置日志级别
void Logger::SetLogLevel(LogLevel level) { loglevel = level; }

//写日志，把日志信息写到lockqueue缓冲区当中
void Logger::Log(std::string msg) { logQueue.Push(msg); }

//停止写日志线程并等待它退出;重复调用安全(exchange 只有第一次生效)
void Logger::Stop() {
  if (stopping.exchange(true)) return;  //已经停过了
  logQueue.Push("");  //哨兵: 只为把线程从 Pop 唤醒,内容不写盘
  if (writeThread.joinable()) writeThread.join();
}

//析构即停机: 进程正常退出时 C++ 运行时自动走到这里
Logger::~Logger() { Stop(); }