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
    std::cout << "do GetFriendList service!" << std::endl;
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
    res->mutable_result()->set_errcode(0);
    res->mutable_result()->set_errmsg("");
    for (const auto& e : friendList) {
      auto temp = res->add_friends();
      *temp = e;
    }

    done->Run();
  }
};

int main(int argc, char* argv[]) {
  // 框架初始化: 从 -i 指定的配置文件读入本机 IP/端口、ZK 地址等
  //   用法示例: provider -i test.conf
  KopirpcApplication::Init(argc, argv);

  // RpcProvider 是框架的网络服务对象: 把 FriendService 发布到 RPC 节点上,
  // 之后大量 caller 的并发请求由 muduo 网络层承载
  RpcProvider provider;
  provider.NotifyService(new FriendService());  // 对象随进程存活,无需释放

  // 启动 RPC 服务节点: Run() 阻塞于此,进程进入事件循环等待远程调用
  // 此后对 caller 来说,远程调 Login 就像调本地方法(网络细节全被框架藏掉)
  provider.Run();

  return 0;
}
