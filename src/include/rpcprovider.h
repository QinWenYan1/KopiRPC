#pragma once
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <muduo/base/Timestamp.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/TcpConnection.h>

#include <unordered_map>

#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"
#include "muduo/net/TcpServer.h"
#include "logger.h"

// RpcProvider —— 框架提供的 RPC 服务发布器
//
// 使用方(callee 业务,如 UserService)两步把本地服务暴露为 RPC 服务:
//   1. NotifyService(service): 登记一个 protobuf 生成的服务对象
//   2. Run(): 启动网络节点,监听端口并响应远程调用(阻塞,不返回)
//
// 设计要点: 框架不依赖任何具体业务——登记接口统一接收基类指针
// google::protobuf::Service*,因此任何 .proto 定义的服务都能发布
class RpcProvider {
 public:
  // 登记一个 RPC 服务对象
  //   service: protobuf 生成的服务派生类实例(必须继承
  //            google::protobuf::Service*);框架不接管其生命周期,
  //            调用方需保证它在 Run() 期间始终存活
  void NotifyService(google::protobuf::Service*);

  // 启动 RPC 服务节点: 读取配置 -> 监听端口 -> 进入事件循环(阻塞)
  void Run();

 private:
  // 事件循环(muduo Reactor 核心),Run() 中驱动网络 IO
  muduo::net::EventLoop eventLoop;

  // 一个服务的注册信息: 服务对象 + 方法表
  struct ServiceInfo {
    google::protobuf::Service* serv;  // 服务对象
    std::unordered_map<std::string,
                       const google::protobuf::MethodDescriptor*>
        methodMap;  // 方法名 -> 方法描述符
  };

  // 服务名 -> 服务注册信息
  std::unordered_map<std::string, ServiceInfo> serviceMap;

  // TCP 连接建立/断开时的回调(注册给 TcpServer,由 muduo 触发)
  void OnConnection(const muduo::net::TcpConnectionPtr&);
  // 连接上有数据可读时的回调(注册给 TcpServer,由 muduo 触发)
  void OnMessage(const muduo::net::TcpConnectionPtr&, muduo::net::Buffer*,
                 muduo::Timestamp);
  // Closure的回调操作，用于序列化rpc的response和网络发送
  void SendRpcResponse(const muduo::net::TcpConnectionPtr&,
                       google::protobuf::Message*);
};
