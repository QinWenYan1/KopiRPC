#include "kopirpccontroller.h"

KopirpcController::KopirpcController() {
  isFailed = false;
  errText = "";
}
void KopirpcController::Reset() {
  isFailed = false;
  errText = "";
}
bool KopirpcController::Failed() const { return isFailed; }
std::string KopirpcController::ErrorText() const { return errText; }
void KopirpcController::SetFailed(const std::string& reason) {
  isFailed = true;
  errText = reason;
}

//目前为实现的具体功能：现在用不到，直接置为空函数
void KopirpcController::StartCancel(){}
bool KopirpcController::IsCandeled() const {return false;}
void NotifyOnCancel(google::protobuf::Closure* callback){}

