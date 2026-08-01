// callee 示例: RPC 服务提供者进程 —— 把 UserService 发布到 RPC 节点上
#include <iostream>
#include <string>

#include "kopirpcapplication.h"
#include "rpcprovider.h"
#include "user.pb.h"

/* 提供一个本地的简单服务*/
class UserService
    : public fixbug::UserServiceRpc  // protoc 由 user.proto 生成的 RPC 接口基类
{
 public:
  // 本地业务方法: 真正的登录逻辑(教学演示: 打印参数并恒返回 true)
  bool Login(std::string name, std::string pwd) {
    std::cout << "Doing the local service: login..." << std::endl;
    std::cout << "Name: " << name << " pwd: " << pwd << std::endl;
    return true;
  }

  bool Register(uint32_t id, std::string name, std::string pwd) {
    std::cout << "Doing the local service: register..." << std::endl;
    std::cout << "id: " << id << " name: " << name << " pwd: " << pwd
              << std::endl;
    return true;
  }

  // RPC 入口: 重写基类虚函数,框架收到远程 Login 请求后派发到这里
  //   调用链: caller 发 LoginRequest -> muduo 网络 -> 框架反序列化 -> 本函数
  //   四个参数的签名由 protobuf 生成的基类固定,业务层只管用不用改
  void Login(::google::protobuf::RpcController* controller,
             const ::fixbug::LoginRequest* request,
             ::fixbug::LoginResponse* response,
             ::google::protobuf::Closure* done) override {
    // 1. 从请求对象取出参数(框架已完成反序列化)
    std::string name = request->name();
    std::string pwd = request->pwd();

    // 2. 调用本地业务方法,完成实际逻辑
    bool login_result = Login(name, pwd);

    // 3. 填写响应: errcode=0 表示无错误,并带上登录结果
    fixbug::ResultCode* code = response->mutable_result();
    code->set_errcode(0);
    code->set_errmsg("");
    response->set_success(login_result);

    // 4. done->Run() 通知框架收尾: 由框架把 response 序列化并发回 caller
    done->Run();
  }

  void Register(google::protobuf::RpcController* controller,
                const ::fixbug::RegisterRequest* req, ::fixbug::RegisterResponse* res,
                ::google::protobuf::Closure* done) override {
    
    // 1. 从请求对象取出参数(框架已完成反序列化)
    uint32_t id = req->id(); 
    std::string name = req->name(); 
    std::string pwd = req->pwd(); 
    
    // 2. 调用本地业务方法,完成实际逻辑
    bool ret = Register(id, name, pwd);

    // 3. 填写响应: errcode=0 表示无错误,并带上登录结果
    res->mutable_result()->set_errcode(0); 
    res->mutable_result()->set_errmsg(""); 
    res->set_success(true);
    
    // 4. done->Run() 通知框架收尾: 由框架把 response 序列化并发回 caller
    done->Run();
  }
};

int main(int argc, char* argv[]) {
  // 框架初始化: 从 -i 指定的配置文件读入本机 IP/端口、ZK 地址等
  //   用法示例: provider -i test.conf
  KopirpcApplication::Init(argc, argv);

  // RpcProvider 是框架的网络服务对象: 把 UserService 发布到 RPC 节点上,
  // 之后大量 caller 的并发请求由 muduo 网络层承载
  RpcProvider provider;
  provider.NotifyService(new UserService());  // 对象随进程存活,无需释放

  // 启动 RPC 服务节点: Run() 阻塞于此,进程进入事件循环等待远程调用
  // 此后对 caller 来说,远程调 Login 就像调本地方法(网络细节全被框架藏掉)
  provider.Run();

  return 0;
}
