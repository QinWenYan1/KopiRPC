#pragma once
#include "google/protobuf/service.h"

//框架提供的专门服务发布rpc服务的网络对象类
#include <google/protobuf/service.h>
class RpcProvider
{
public:
    /*
    * 我们开发的是框架，然而框架一定不能依赖于具体某个业务
    * 也就是说不能只是能发布UserServerice类的服务
    * 还可以发布UserService基类Service类的服务指针
    * 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
    */
    void NotifyService(google::protobuf::Service*); 
};