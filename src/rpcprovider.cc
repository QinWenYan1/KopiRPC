#include "rpcprovider.h"
#include <google/protobuf/descriptor.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <functional>
#include <string>
#include "kopirpcapplication.h"

//这是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service) {
  //获取服务对象的描述信息
  const google::protobuf::ServiceDescriptor* serviceDescPtr =
      service->GetDescriptor();
  //获取服务的名字
  std::string service_name = serviceDescPtr->name();
  //获取类对象方法的数量
  int methodCnt = serviceDescPtr->method_count();
}

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
  muduo::net::TcpServer server(&eventLoop, address, "RPCProvider");
  //绑定回调和消息读写回调方法
  // muduo帮我们分离了网络代码和业务代码
  //我们只需要关注有没有链接，以及有没有新的读写事件
  server.setConnectionCallback(
      [this](const muduo::net::TcpConnectionPtr& conn) {
        this->onConnection(conn);
      });

  server.setMessageCallback(
      [this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf,
             muduo::Timestamp t) { this->onMessage(conn, buf, t); });

  //设置muduo库的线程数量
  server.setThreadNum(4);

  std::cout << "RPC Provider start service at IP: " << ip << " Port: " << port
            << std::endl;

  //启动网络服务
  server.start();
  //以阻塞方式等待远程连接
  eventLoop.loop();
}

//新来的socket的链接事件的回调
void RpcProvider::onConnection(const muduo::net::TcpConnectionPtr& conn) {}

// TCP 已经建立连接用户的读写回调
void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buf, muduo::Timestamp t) {}