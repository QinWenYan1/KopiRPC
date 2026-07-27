#include "rpcprovider.h"

#include <muduo/net/InetAddress.h>

#include <string>

#include "kopirpcapplication.h"

void RpcProvider::NotifyService(google::protobuf::Service*) {}

void RpcProvider::Run() {
  //先将配置拿出并配置好
  std::string ip =
      KopirpcApplication::GetInstance().GetConfigFile().Load("rpcserverip");
  uint16_t port = atoi(KopirpcApplication::GetInstance()
                           .GetConfigFile()
                           .Load("rpcserverport")
                           .c_str());
  muduo::net::InetAddress address(ip, port);
  //启动TCP 服务器
}
