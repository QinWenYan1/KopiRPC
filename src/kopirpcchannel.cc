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
#include <mutex>
#include <string>
#include <unordered_map>

#include "kopiconnectpool.h"
#include "kopirpcapplication.h"
#include "logger.h"
#include "netutil.h"
#include "rpcheader.pb.h"
#include "zookeeperutil.h"


// d2: ZK 地址缓存 —— methodPath -> "ip:port"
// 命中则跳过整个 ZK 会话(Start+GetData 是一次完整 TCP 会话,每次调用的大头开销)
// mutex 护 map: CallMethod 会被多线程并发调(bench 就是 8 线程)
// 已知限制(挂账): 缓存不过期 —— provider 换地址/重启后旧缓存仍在,失效/watcher 留后续
static std::unordered_map<std::string, std::string> s_addrCache;
static std::mutex s_addrCacheMtx;


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
    if (controller) controller->SetFailed("serialize rpc header error!");
    return;
  }

  //组织等待发送的字符串
  std::string sendRpcStr;
  //放header size
  sendRpcStr.append(reinterpret_cast<char*>(&headerSize), sizeof(headerSize));
  sendRpcStr += rpcHeaderStr;  //放header
  sendRpcStr += argsStr;       //放args Str

  // 打印调试信息(rpcHeaderStr/argsStr 是 protobuf 二进制字节,只打可读字段)
  LOG_INFO("header size:%u service:%s method:%s", headerSize,
           serviceName.c_str(), methodName.c_str());

  //使用tcp编程发送字符流，完成rpc的远程调用
  // int clientfd = socket(AF_INET, SOCK_STREAM, 0);
  // if (clientfd == -1) {
  //  std::string errtxt = "create socket error! errno: ";
  //  errtxt += std::to_string(errno);
  //  if (controller) controller->SetFailed(errtxt);
  // return;
  //}

  // "/UserService/Login"
  std::string methodPath = "/" + serviceName + "/" + methodName;

  // d2: 先查缓存,未命中才开 ZK 会话;锁只护 map 读写两行,ZK 网络查询绝不持锁
  std::string data;
  {
    std::lock_guard<std::mutex> lock(s_addrCacheMtx);
    auto it = s_addrCache.find(methodPath);
    if (it != s_addrCache.end()) data = it->second;
  }
  if (data.empty()) {
    // 从 zk server 读取到服务所在 ip 和 port
    ZkClient zkCli;
    zkCli.Start();
    // 从指定路径拿到指定数据 -> 127.0.0.1:8000
    data = zkCli.GetData(methodPath.c_str());  // 从指定路径拿到数据 -> 127.0.0.1:8000
    if (!data.empty()) {
      std::lock_guard<std::mutex> lock(s_addrCacheMtx);
      s_addrCache[methodPath] = data;
    }
  }

  // 缓存也没有数据，zookeeper 服务器也没有数据
  if (data == "") {
    controller->SetFailed(methodPath + " is not existed!");
    // close(clientfd);  // 正确释放client fd
    return;
  }

  int idx = data.find(":");
  if (idx == -1) {
    controller->SetFailed(methodPath + " address is invalid!");
    // close(clientfd);
    return;
  }
  std::string ip = data.substr(0, idx);
  uint16_t port = atoi(data.substr(idx + 1, data.size() - idx).c_str());

  // sockaddr_in serverAddr;
  // serverAddr.sin_family = AF_INET;
  // serverAddr.sin_port = htons(port);
  // serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

  // rpc服务节点链接启动
  // if (connect(clientfd, reinterpret_cast<sockaddr*>(&serverAddr),
  //            sizeof(serverAddr)) == -1) {
  //  std::string errtxt = "connection error! errno: ";
  //  errtxt += std::to_string(errno);
  //  if (controller) controller->SetFailed(errtxt);
  //  close(clientfd);
  //  return;
  //}

  // d1 池化动机: 旧版每次调用 socket()+connect() 新建短连接,用完 close
  //   —— 每次白付一次 TCP 握手/挥手,高并发下还制造海量 TIME_WAIT 耗尽源端口。
  //   现在改为向连接池借: 有现成空闲连接直接复用,没有才新建(动机详见
  //   kopiconnectpool.h)。
  //
  //   借还铁律(像 new/delete 配对): 从借到这一刻起,本函数每条 return 路径
  //   都必须归还 —— 通信失败的还 isBad=true(池焚毁,防坏连接污染复用),
  //   一切顺利的还 isBad=false(健康回队,下次调用接着用)。
  //   从借到还之间,本函数不再出现 close(clientfd): 关连接是池的内部职责。
  int clientfd = KopiConnectPool::GetInstance().BorrowConnection(ip, port);
  if (clientfd == -1) {
    /* SetFailed("borrow error") + return,不用 close */
    std::string errtxt = "Borrow connection error! errno: ";
    errtxt += std::to_string(errno);
    if (controller) controller->SetFailed(errtxt);
    return;
  }

  //发送rpc请求
  // 发送失败 = 连接已受伤 → 还 isBad=true 焚毁;下同(RecvN/parse 失败同理)
  // parse 失败虽"帧收对了",但 caller 分不清是内容脏还是流早已错位
  // —— 错位会留在流里毒害下个借用者,说不清就焚,代价仅一次重建
  if (SendN(clientfd, sendRpcStr.c_str(), sendRpcStr.size()) == false) {
    std::string errtxt = "send error! errno: ";
    errtxt += std::to_string(errno);
    if (controller) controller->SetFailed(errtxt);
    KopiConnectPool::GetInstance().ReturnConnection(ip, port, clientfd, true);
    return;
  }

  // std::string responseStr;
  // char buf[1024];
  // ssize_t n = 0;
  // while ((n = recv(clientfd, buf, sizeof(buf), 0)) > 0) {
  //   responseStr.append(buf, n);  //问题排除： string(buf, n,
  //   size)导致的阶段出错
  // }
  // if (n == -1) {
  //  std::string errtxt = "receiving error: ";
  //  errtxt += std::to_string(errno);
  //  if (controller) controller->SetFailed(errtxt);
  //  close(clientfd);
  //  return;
  //}

  // 收响应帧: 先读 4 字节 respSize,再读 respSize 字节 body
  // —— 与 provider SendRpcResponse 是同一纸契约的两端(动机见 rpcprovider.cc):
  //   旧版"recv 至 0 等 EOF"已随池化废除(连接不再关闭,永远等不到 0)
  uint32_t respSize = 0;
  if (!RecvN(clientfd, reinterpret_cast<char*>(&respSize), sizeof(respSize))) {
    std::string errtxt = "recv response size error! errno: ";
    errtxt += std::to_string(errno);
    if (controller) controller->SetFailed(errtxt);
    KopiConnectPool::GetInstance().ReturnConnection(ip, port, clientfd, true);
    return;
  }

  std::string responseStr(respSize, '\0');
  if (!RecvN(clientfd, &responseStr[0], respSize)) {
    std::string errtxt = "recv response body error! errno: ";
    errtxt += std::to_string(errno);
    if (controller) controller->SetFailed(errtxt);
    KopiConnectPool::GetInstance().ReturnConnection(ip, port, clientfd, true);
    return;
  }

  //反序列化rpc调用的响应数据
  /*
   * 问题排除：ParseFromString为用户态操作，errno是内核态错误搬运，
   *         ParseFromString的报错不会被记录
   */
  if (!response->ParseFromString(responseStr)) {
    std::string errtxt = "parse error! response string: ";
    errtxt += responseStr;
    if (controller) controller->SetFailed(errtxt);
    KopiConnectPool::GetInstance().ReturnConnection(ip, port, clientfd, true);
    return;
  }
  // 一切顺利: isBad=false 健康归还,连接回桶等待下次复用
  KopiConnectPool::GetInstance().ReturnConnection(ip, port, clientfd, false);
}