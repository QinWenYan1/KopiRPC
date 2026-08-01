#include <iostream>

#include "friend.pb.h"
#include "kopirpcapplication.h"
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
  // stub里面传入一个channel，通过派生类实现多态调用
  fixbug::FriendServiceRpc_Stub stub(new KopiRpcChannel());
  // rpc方法的请求参数
  fixbug::GetFriendListRequest request;
  request.set_userid(1000);
  // rpc方法的响应
  fixbug::GetFriendListResponse resp;
  //发起rpc方法的调用，同步rpc调用过程，Login底层调用Kopirpcchannel::callmethod
  //同步阻塞的方式调用：client 发送请求并等待response
  stub.GetFriendList(nullptr, &request, &resp, nullptr);  //跑到channel里面执行

  //一次rpc调用完成，读调用结果
  if (resp.result().errcode() == 0) {
    std::cout << "Get friend list: " << std::endl;

    for (int i = 0; i != resp.friends_size(); ++i) {
      std::cout << "Name: " << resp.friends(i) << std::endl;
    }
  } else {
    std::cout << "rpc login response error: " << resp.result().errmsg()
              << std::endl;
  }

  return 0;
}