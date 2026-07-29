#include "rpcprovider.h"

#include <google/protobuf/descriptor.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>

#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

#include "kopirpcapplication.h"
#include "rpcheader.pb.h"
/*
 * servName对应一个service描述符
 *       service描述符对应一个或多个method 方法描述符（或者没有）
 *
 * json: 文本结构，key-value对
 * profobuf: 二进制，还能抽象方法
 */

//这是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service) {
  ServiceInfo servInfo;
  //获取服务对象的描述信息
  const google::protobuf::ServiceDescriptor* serviceDescPtr =
      service->GetDescriptor();
  //获取服务的名字
  std::string servName = serviceDescPtr->name();
  //获取类对象方法的数量
  int methodCnt = serviceDescPtr->method_count();
  //打印信息
  std::cout << "Service name: " << servName << std::endl;

  //将方法+方法名字注册到该服务的映射信息中
  for (int i = 0; i < methodCnt; ++i) {
    //获取服务对象指定下标的服务方法的描述（抽象描述）
    const google::protobuf::MethodDescriptor* methodDescPtr =
        serviceDescPtr->method(i);
    std::string methodName = methodDescPtr->name();
    servInfo.methodMap.insert({methodName, methodDescPtr});
    std::cout << "method name: " << methodName << std::endl;
  }

  //将该服务的信息注册到provider中
  servInfo.serv = service;
  serviceMap.insert({servName, servInfo});
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
void RpcProvider::onConnection(const muduo::net::TcpConnectionPtr& conn) {
  if (!conn->connected()) {
    //和rpc client的链接断开了
    conn->shutdown();
  }
}

/*
 * 在框架内部，RpcProvider和RpcConsumer需要协商好通讯之间protobuf数据类型
 * 在RpcProvider中，我们通过：
 *   1. Service name 找到 Service
 *   2. Method name 找到 Method
 *   3. args 调用 Method
 * 所以需要这三个信息，但是需要去判断序列化后哪一段时Service name，Method name
 * 因此通过proto中的message的定义定义一个数据头，来区分
 * 同时还有一个问题由于请求时连续的，会有TCP粘包现象
 * 因此还要记录args段的长度，来区分message的尾部
 */

// 已经建立连接用户的读写回调,如果cient有一个rpc服务请求时，onMessage就会响应
void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buf, muduo::Timestamp t) {
  //网络上接受的远程rpc调用请求的字符流，包含了1.函数名，2.参数
  std::string recvBuf = buf->retrieveAllAsString();

  //从字符流中读取前4个字节的字符流的内容
  /*
   * message 从不在自己肚子里记自己的长度——长度永远写在"外面一层"：嵌套的靠父
   * message 记，最外层的没人替它记，所以框架手动在开头贴上 4 个字节
   */
  uint32_t headerSize = 0;
  std::memcpy(&headerSize, recvBuf.data(), sizeof(headerSize));

  //根据Header size 读取数据头的原始数据流: 掠过前4个字节得到rpc请求的详细信息
  std::string rpcHeaderStr = recvBuf.substr(4, headerSize);
  kopirpc::RpcHeader rpcHeader;
  std::string serviceName, methodName;
  uint32_t argsSize = 0;
  if (rpcHeader.ParseFromString(rpcHeaderStr)) {
    //数据化反序列成功
    serviceName = rpcHeader.servicename();
    methodName = rpcHeader.methodname();
    argsSize = rpcHeader.argssize();
  } else {
    //数据反序列化失败
    std::cout << "rpc Header " << rpcHeaderStr << " parse error" << std::endl;
    return;
  }
  //获取rpc方法参数的字符流数据
  std::string argsStr = recvBuf.substr(headerSize + 4, argsSize);
  //打印调试信息
  std::cout << "==============================================" << std::endl;
  std::cout << "header size: " << headerSize << std::endl;
  std::cout << "rpc header content: " << rpcHeaderStr << std::endl;
  std::cout << "service name: " << serviceName << std::endl;
  std::cout << "method name: " << methodName << std::endl;
  std::cout << "args str: " << argsStr << std::endl;
  std::cout << "==============================================" << std::endl;
}