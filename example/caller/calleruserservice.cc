#include <iostream>

#include "KopirpcApplication.h"
#include "user.pb.h"
#include "kopirpcchannel.h"

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
  //stub里面传入一个channel，通过派生类实现多态调用
  fixbug::UserServiceRpc_Stub stub(new KopiRpcChannel()); 
  //stub.login(); 跑到channel里面执行

  return 0;
}