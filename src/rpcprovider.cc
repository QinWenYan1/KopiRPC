#include "rpcprovider.h"
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <string>
#include "kopirpcapplication.h"
#include <functional>

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
  //启动 TCP 服务器对象
  muduo::net::TcpServer server(eventLoop, address,"RPCProvider"); 
  //绑定回调和消息读写回调方法
  //muduo帮我们分离了网络代码和业务代码
  //我们只需要关注有没有链接，以及有没有新的读写事件
  //server.setConnectionCallback();


  //设置muduo库的线程数量
  server.setThreadNum(4); 
}

  //新来的socket的链接事件的回调
void RpcProvider::onConnection(const muduo::net::TcpConnection&){

}
