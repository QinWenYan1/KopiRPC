#pragma once
#include <google/protobuf/arena.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

class KopiRpcChannel : public google::protobuf::RpcChannel {
 public:
  //所有通过stub代理的对象调用rpc方法，都走到这里了，统一做rpc方法的数据序列化和网络发送
  void CallMethod(const google::protobuf::MethodDescriptor*,
                  google::protobuf::RpcController*,
                  const google::protobuf::Message*,
                  google::protobuf::Message*,
                  google::protobuf::Closure*);
};