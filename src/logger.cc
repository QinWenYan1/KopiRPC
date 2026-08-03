#include "logger.h"

//获取日志的单例
Logger& Logger::GetInstance(){} 
//设置日志级别
void Logger::SetLogLevel(LogLevel level){}
//写日志
void Logger::Log(std::string msg){}