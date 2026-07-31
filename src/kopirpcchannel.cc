#include "kopirpcchannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

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
    std::cout << "seralize request error!" << std::endl;
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
    std::cout << "seralize rpc header error!" << std::endl;
    return; 
  }

  //组织等待发送的字符串 
}