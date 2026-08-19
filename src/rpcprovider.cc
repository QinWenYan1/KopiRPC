#include "rpcprovider.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <muduo/net/Callbacks.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <zookeeper/zookeeper.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

#include "kopirpcapplication.h"
#include "logger.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"

// 现在需要自定义回调对象了，因为protobuf默认的并不支持传入5个及其以上的
// protobuf 的 Closure 是抽象类,只需实现 Run()
class KopiClosure : public google::protobuf::Closure {
 public:
  explicit KopiClosure(std::function<void()> fn) : fn_(std::move(fn)) {}
  void Run() override {
    fn_();
    delete this;
  }  //跑完自杀,对齐 NewCallback 的行为,done 本身不泄漏
 private:
  std::function<void()> fn_;
};

/*
 * servName对应一个service描述符
 *       service描述符对应一个或多个method 方法描述符（或者没有）
 *
 * json: 文本结构，key-value对
 * protobuf: 二进制，还能抽象方法
 */

// 这是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service* service) {
  ServiceInfo servInfo;

  // 借助 protobuf 反射拿到服务描述符: 从中取服务名和全部方法描述
  const google::protobuf::ServiceDescriptor* serviceDescPtr =
      service->GetDescriptor();
  std::string servName = serviceDescPtr->name();
  int methodCnt = serviceDescPtr->method_count();
  LOG_INFO("Service name: %s", servName.c_str());

  // 把每个方法以 方法名 -> 方法描述符 登记进 methodMap
  for (int i = 0; i < methodCnt; ++i) {
    const google::protobuf::MethodDescriptor* methodDescPtr =
        serviceDescPtr->method(i);
    std::string methodName = methodDescPtr->name();
    servInfo.methodMap.insert({methodName, methodDescPtr});
    LOG_INFO("Method name: %s", methodName.c_str());
  }

  // 整个服务以 服务名 -> 服务信息 登记进注册表
  servInfo.serv = service;
  serviceMap.insert({servName, servInfo});
}

void RpcProvider::Run() {
  // 从配置文件读出本节点的 ip 和 port
  std::string ip =
      KopirpcApplication::GetInstance().GetConfigFile().Load("rpcserverip");
  uint16_t port = atoi(KopirpcApplication::GetInstance()
                           .GetConfigFile()
                           .Load("rpcserverport")
                           .c_str());
  muduo::net::InetAddress address(ip, port);

  // 创建 TCP 服务器并注册回调: muduo 把网络 IO 和业务代码分离,
  // 框架只需关心"连接有没有建立/断开"和"有没有数据可读"
  muduo::net::TcpServer server(&eventLoop, address, "RPCProvider");
  server.setConnectionCallback(
      [this](const muduo::net::TcpConnectionPtr& conn) {
        this->OnConnection(conn);
      });

  server.setMessageCallback(
      [this](const muduo::net::TcpConnectionPtr& conn, muduo::net::Buffer* buf,
             muduo::Timestamp t) { this->OnMessage(conn, buf, t); });

  // 设置 muduo 的 IO 线程数(多 Reactor 线程池大小)
  server.setThreadNum(4);

  LOG_INFO("RPC Provider start service at IP: %s, Port: %d", ip.c_str(), port);

  //把当前rpc节点上咬发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
  //会自动发送pin心跳消息保持与zk client的链接
  ZkClient zkCli;
  zkCli.Start();
  // service永久性节点 method_name为临时性节点
  //  session timeout 30s zkclient API, 起一个网络IO线程 1/3 * timeout
  //  时间发送ping消息
  for (auto& sp : serviceMap) {
    //先组成 "/service_name" 这个路径，比如: "/UserServiceRpc"
    std::string servicePath = "/" + sp.first;
    zkCli.Create(servicePath.c_str(), nullptr, 0);
    for (auto& mp : sp.second.methodMap) {
      // 然后再组成 "/service_name/method_name" 如 "/UserServiceRpc/Login"
      std::string methodPath = servicePath + "/" + mp.first;
      char data[128] = {0};
      sprintf(data, "%s:%d", ip.c_str(), port);
      //服务的方法都是临时性节点: ZOO_EPHEMERAL
      zkCli.Create(methodPath.c_str(), data, strlen(data), ZOO_EPHEMERAL);
    }
  }

  // 启动网络服务,进入事件循环(阻塞,此后一切由回调驱动)
  server.start();
  eventLoop.loop();
}

// 连接建立/断开时触发
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr& conn) {
  if (!conn->connected()) {
    // 和 rpc client 的连接断开了
    conn->shutdown();
  }
}

/*
 * provider 和 caller 约定好的私有协议帧格式:
 *   [4字节 headerSize][headerSize 字节的 RpcHeader][argsSize 字节的 args]
 * 一帧必须能回答三个问题: 调哪个 service、哪个 method、参数是什么;
 * 而 TCP 是字节流会粘包,所以还要回答"每段在哪结束":
 *   帧首固定 4 字节给出 RpcHeader 的边界,RpcHeader 里的 argsSize 给出整帧的边界
 */

// 连接上有数据可读时触发: 解析一帧 RPC 请求
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                            muduo::net::Buffer* buf, muduo::Timestamp t) {
  // 取出本次收到的全部字节(粘包时可能不止一帧,当前按一帧解析)
  std::string recvBuf = buf->retrieveAllAsString();

  /*
   * message 从不在自己肚子里记自己的长度——长度永远写在"外面一层":嵌套的靠父
   * message 记,最外层的没人替它记,所以框架手动在开头贴上 4 个字节
   */
  uint32_t headerSize = 0;
  std::memcpy(&headerSize, recvBuf.data(), sizeof(headerSize));

  // 按 headerSize 切出 RpcHeader 段并反序列化,得到 service/method 名和 argsSize
  std::string rpcHeaderStr = recvBuf.substr(4, headerSize);
  kopirpc::RpcHeader rpcHeader;
  std::string serviceName, methodName;
  uint32_t argsSize = 0;
  if (rpcHeader.ParseFromString(rpcHeaderStr)) {
    serviceName = rpcHeader.servicename();
    methodName = rpcHeader.methodname();
    argsSize = rpcHeader.argssize();
  } else {
    // 请求头损坏,丢弃本次请求
    LOG_ERR("rpc Header parse error");
    return;
  }

  // 按 argsSize 切出参数段(caller 传来的请求 message 的序列化字节)
  std::string argsStr = recvBuf.substr(headerSize + 4, argsSize);

  // 打印调试信息(rpcHeaderStr/argsStr 是 protobuf 二进制字节,只打可读字段)
  LOG_INFO("header size:%u service:%s method:%s", headerSize,
           serviceName.c_str(), methodName.c_str());

  //获取service对象和method对象
  auto item = serviceMap.find(serviceName);
  if (item == serviceMap.end()) {
    LOG_ERR("%s is not existed!", serviceName.c_str());
    return;
  }

  auto servInfo = item->second;
  //对应要调用的user service，获取service对象 UserService
  google::protobuf::Service* service = servInfo.serv;
  auto itemMethod = servInfo.methodMap.find(methodName);
  if (itemMethod == servInfo.methodMap.end()) {
    LOG_ERR("%s's %s is not existed!", serviceName.c_str(), methodName.c_str());
    return;
  }

  //对应要调用的method：login
  const google::protobuf::MethodDescriptor* method = itemMethod->second;

  //但是还要生成方法调用的请求和响应参数从argsStr
  /* 这里我们直接使用堆分配，但是 SendRpcResponse 里没有任何
   * delete——全程搜不到，泄漏坐实 */
  google::protobuf::Message* request =
      service->GetRequestPrototype(method).New();
  if (!request->ParseFromString(argsStr)) {
    delete request; 
    LOG_ERR("request parse error!");
    return;
  }
  google::protobuf::Message* response =
      service->GetResponsePrototype(method).New();

  //给下面的method方法的调用，绑定一个回调函数 closure done 给CallMethod
  google::protobuf::Closure* done = new KopiClosure([this,conn,response,request](){
    this->SendRpcResponse(conn, response, request); 
  }); 

  //在框架上根据远端rpc请求，调用当前rpc节点上发布的方法
  //实现：new UserService().login(controller, request, response, done)
  service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的response和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr& conn,
                                  google::protobuf::Message* res,
                                  google::protobuf::Message* req) {
  std::string responseStr;
  // response进行序列化，序列化成功后，通过网络把rpc方法执行的结果发送回rpc调用方
  if (res->SerializeToString(&responseStr)) {
    conn->send(responseStr);
  } else {
    LOG_ERR("Serialize Response error!");
  }
  conn->shutdown();  //模拟http的短链接服务，由rpcprovider主动断开链接
  delete res;
  delete req;
}