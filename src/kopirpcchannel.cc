#include "kopirpcchannel.h"

#include <arpa/inet.h>
#include <errno.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>

#include "kopirpcApplication.h"
#include "rpcheader.pb.h"

/*
 * 数据格式：
 *   [header size ][service name][method name][args size] | [argsStr]
 * 当header装载好了之后就可以发送过去了
 */
void KopiRpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done) {
  const google::protobuf::ServiceDescriptor* sd = method->service();
  std::string serviceName = sd->name();
  std::string methodName = method->name();

  //获取参数的序列化字符串长度
  std::string argsStr;
  int argsSize = 0;
  if (request->SerializeToString(&argsStr)) {
    argsSize = argsStr.size();
  } else {
    //如果传入了controller就设置为失败
    if (controller) controller->SetFailed("serialize request error!");
    return;
  }

  //定义rpc的请求header
  kopirpc::RpcHeader rpcHeader;
  rpcHeader.set_methodname(methodName);    // method name有了
  rpcHeader.set_servicename(serviceName);  // service name有了
  rpcHeader.set_argssize(argsSize);        // args Str 有了

  uint32_t headerSize = 0;  // header size有了
  std::string rpcHeaderStr;
  if (rpcHeader.SerializeToString(&rpcHeaderStr)) {
    headerSize = rpcHeaderStr.size();
  } else {
    //如果传入了controller就设置为失败
    if (controller) controller->SetFailed("seralize rpc header error!");
    return;
  }

  //组织等待发送的字符串
  std::string sendRpcStr;
  //放header size
  sendRpcStr.append(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));
  sendRpcStr += rpcHeaderStr;  //放header
  sendRpcStr += argsStr;       //放args Str

  // 打印调试信息
  std::cout << "==============================================" << std::endl;
  std::cout << "header size: " << headerSize << std::endl;
  std::cout << "rpc header content: " << rpcHeaderStr << std::endl;
  std::cout << "service name: " << serviceName << std::endl;
  std::cout << "method name: " << methodName << std::endl;
  std::cout << "args str: " << argsStr << std::endl;
  std::cout << "==============================================" << std::endl;

  //使用tcp编程发送字符流，完成rpc的远程调用
  int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  if (clientfd == -1) {
    std::string errtxt = "create socket error! errno: ";
    errtxt += errno;
    if (controller) controller->SetFailed(errtxt);
    return;
  }

  std::string ip =
      KopirpcApplication::GetInstance().GetConfigFile().Load("rpcserverip");
  uint16_t port = atoi(KopirpcApplication::GetInstance()
                           .GetConfigFile()
                           .Load("rpcserverport")
                           .c_str());

  sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(port);
  serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

  // rpc服务节点链接启动
  if (connect(clientfd, reinterpret_cast<sockaddr*>(&serverAddr),
              sizeof(serverAddr)) == -1) {
    std::string errtxt = "connection error! errno: ";
    errtxt += errno;
    if (controller) controller->SetFailed(errtxt);
    close(clientfd);
    return;
  }

  //发送rpc请求
  if (send(clientfd, sendRpcStr.c_str(), sendRpcStr.size(), 0) == -1) {
    std::string errtxt = "end error! errno: ";
    errtxt += errno;
    if (controller) controller->SetFailed(errtxt);
    close(clientfd);
    return;
  }

  // 循环接收,直到对端关闭连接(recv 返回 0)
  std::string responseStr;
  char buf[1024];
  ssize_t n = 0;
  while ((n = recv(clientfd, buf, sizeof(buf), 0)) > 0) {
    responseStr.append(buf, n);  // string(buf, n size)引出的一个问题
  }
  if (n == -1) {
    std::cout << "reciving error! " << errno << std::endl;
    close(clientfd);
    return;
  }

  //反序列化rpc调用的响应数据
  if (!response->ParseFromString(responseStr)) {
    std::cout << "parse error! response string: " << responseStr << std::endl;
    close(clientfd);
    return;
  }
  close(clientfd);
}