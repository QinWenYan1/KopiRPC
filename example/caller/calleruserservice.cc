#include <iostream>

#include "Kopirpcapplication.h"
#include "kopirpcchannel.h"
#include "user.pb.h"

/*
 * example属于是业务代码，包括caller + callee
 */

int main(int argc, char* argv[]) {
  //整个程序启动以后，想要使用KopiRpc框架服务，一定要调用框架的初始化函数
  //只需要初始化一次
  KopirpcApplication::Init(argc, argv);

  //演示调用远程发布的rpc方法login
  // caller -> stub -> RpcChannel
  // RpcChannel -> RpcChannel::callMethod
  // 集中来做所有的rpc方法调用的参数序列化和网络发送
  //但是RpcChannel是虚类，需要用户自己手动实现
  // stub里面传入一个channel，通过派生类实现多态调用
  fixbug::UserServiceRpc_Stub stub(new KopiRpcChannel());
  // rpc方法的请求参数
  fixbug::LoginRequest request;
  request.set_name("zhang san");
  request.set_pwd("123456");
  // rpc方法的响应
  fixbug::LoginResponse resp;
  //发起rpc方法的调用，同步rpc调用过程，Login底层调用Kopirpcchannel::callmethod
  //同步阻塞的方式调用
  stub.Login(nullptr, &request, &resp, nullptr);  //跑到channel里面执行

  //一次rpc调用完成，读调用结果
  if (resp.result().errcode() == 0) {
    std::cout << "rpc login response: " << resp.success() << std::endl;
  } else {
    std::cout << "rpc login response error: " << resp.result().errmsg()
              << std::endl;
  }

  return 0;
}