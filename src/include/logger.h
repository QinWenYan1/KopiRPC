#pragma once 
#include "lockqueue.h"
#include <string>

enum LogLevel{
    INFO, //普通信息
    ERROR,//错误信息
}; 
//kopirpc框架提供的日志系统
class Logger{
public:
    int loglevel; //记录日志级别
    LockQueue<std::string> logQueue; 
}; 