#include <iostream>
#include <string>
#include "user.pb.h"
#include "KopirpcApplication.h"


/* 提供一个本地的简单服务*/
class UserService : public fixbug::UserServiceRpc //使用rpc服务发布端（rpc服务提供者）
{ 
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "Doing the local service: login..."<< std::endl; 
        std::cout << "Name: " << name << " pwd: " << pwd << std::endl; 
        return true;
    }

    
    /*
    * 重写基类UserServiceRpc的徐函数，下面这些方法是框架直接调用的
    * 1. caller ===> login(loginrequest) ===> muduo ===> callee
    * 2. callee 根据接收到的远端想调用login的请求 ===> 交到下面重写的login方法了
    */
    void Login(::google::protobuf::RpcController* controller,
                         const ::fixbug::LoginRequest* request,
                         ::fixbug::LoginResponse* response,
                         ::google::protobuf::Closure* done)
    {
        //1. 框架给业务上报了请求参数loginRequest，业务获取相应数据做本地业务
        std::string name = request -> name(); 
        std::string pwd = request -> pwd(); 

        //2. 做本地业务
        bool login_result = Login(name,pwd); 

        //3. 把响应写入: 这里表示没有错误
        fixbug::ResultCode* code = response->mutable_result();
        code->set_errorcode(0); 
        code->set_errmsg("");
        response->set_sucess(login_result); 

        //4. 执行回调操作：将login repsonse响应的数据序列化传回远端，通过框架完成的
        done->Run(); 
    }
    
}; 


int main(int argc, char** argv)
{   
    //调用框架的初始化操作
    //KopirpcApplication::Init(argc, argv); 

    //可以在框架上发布服务的角色：provider是一个网络服务对象
    //把UserService对象发到rpc节点上
    // RpcProvider provider; 
    // provider.NotifyService(new UserService());

    //启动一个rpc服务发布节点
    //Run以后，进程进入阻塞状态，等待远程rpc调用请求
    //这里相当于UserService的void login()
    //provider.Run(); 

    return 0; 
}
