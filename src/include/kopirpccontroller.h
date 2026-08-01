#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

#include <string>

class KopirpcController {
 public:
  KopirpcController();
  void Reset();
  bool Failed() const;
  std::string ErrorText() const;
  //当出现错误的时候设置，一起将error text也一起设置
  void SetFailed(const std::string& reason);

  //目前不需要去实现的功能
  void StartCancel();
  bool IsCandeled() const;
  void NotifyOnCancel(google::protobuf::Closure* callback);

 private:
  bool isFailed;          // RPC方法执行过程中的状态
  std::string errText;  // RPC方法执行过程中的错误信息
};