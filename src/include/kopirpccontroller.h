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
  void SetFailed(const std::string& reason);

  //目前不需要去实现的功能
  void StartCancel();
  bool IsCandeled() const;
  void NotifyOnCancel(google::protobuf::Closure* callback);
};