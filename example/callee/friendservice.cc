#include <iostream>
#include <string>
#include <vector>

#include "friend.pb.h"
#include "kopirpcapplication.h"
#include "rpcprovider.h"

class FriendService : public fixbug::FriendServiceRpc {
 public:
  //做本地业务
  std::vector<std::string> GetFriendList(uint32_t userid) {
    std::cout << "do GetFriendList service!";
    std::vector<std::string> vec;
    vec.push_back("qinwen");
    vec.push_back("hafid");
    vec.push_back("ada");
    return vec;
  }

  //重写基类方法
  void GetFriendList(::google::protobuf::RpcController* controller,
                     const ::fixbug::GetFriendListRequest* req,
                     ::fixbug::GetFriendListResponse* res,
                     ::google::protobuf::Closure* done) {
    uint32_t userid = req->userid(); 
    std::vector<std::string> friendList = GetFriendList(userid); 

                     }
};