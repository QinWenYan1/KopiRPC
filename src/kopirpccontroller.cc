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