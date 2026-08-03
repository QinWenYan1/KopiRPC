#pragma once 
#include "lockqueue.h"
#include <string>

enum LogLevel{
    INFO, //普通信息
    ERROR,//错误信息
}; 
//kopirpc框架提供的日志系统：使用单例模式
class Logger{
public:
    //获取日志的单例
    static Logger& GetInstance(); 
    //设置日志级别
    void SetLogLevel(LogLevel level); 
    //写日志
    void Log(std::string msg); 
private:
    int loglevel; //记录日志级别
    LockQueue<std::string> logQueue; //日志缓冲队列

    Logger() = default; 
    Logger(const Logger&) = delete; 
    Logger(Logger&&) = delete; 
}; 