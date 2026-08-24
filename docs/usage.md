# KopiRPC 使用文档(USAGE)

> 适用版本:v0.1.0 | 发布包 = include/ + lib/libKopiRPC.a + 本文档 + LICENSE
> 仓库:https://github.com/QinWenYan1/KopiRPC

## 1. 依赖

| 依赖 | 版本 | 用途 |
|---|---|---|
| Protobuf | 3.12.4 | 序列化;.proto 代码生成(protoc) |
| Muduo | — | Reactor 网络库 |
| ZooKeeper C 客户端 | zookeeper_mt | 服务注册与发现 |
| C++ 标准 | C++17 | |

> 💡 偷懒方案:开发镜像 `cspenguin/qwenrpc-dev`(Ubuntu 22.04 / arm64)已预装全部依赖。

## 2. 集成进你的项目(CMake)

```cmake
include_directories(<包路径>/include)
link_directories(<包路径>/lib)
target_link_libraries(你的目标 KopiRPC protobuf muduo_net muduo_base zookeeper_mt pthread)
```

> ⚠️ 静态库链接顺序不能乱:使用者在先,被依赖者在后。

## 3. 四个 API 就够用

| 角色 | 调用 | 作用 |
|---|---|---|
| 双方启动 | `KopirpcApplication::Init(argc, argv)` | 读 `-i` 指定的配置文件 |
| 服务方 | `provider.NotifyService(new YourService())` | 发布服务 |
| 服务方 | `provider.Run()` | 阻塞进入事件循环 |
| 调用方 | `XxxRpc_Stub stub(new KopiRpcChannel())` + `KopirpcController` | 像本地调用一样发 RPC |

## 4. 最小示例(friend 服务走读)

完整可跑源码见仓库 [example/](https://github.com/QinWenYan1/KopiRPC/tree/main/example);这里只贴骨架。

**① .proto 定义**(example/friend.proto):

```proto
syntax = "proto3";
package fixbug;
option cc_generic_services = true;   // 必须:生成 Service 基类与 Stub

message ResultCode { int32 errcode = 1; bytes errmsg = 2; }
message GetFriendListRequest { uint32 userid = 1; }
message GetFriendListResponse { ResultCode result = 1; repeated bytes friends = 2; }

service FriendServiceRpc {
  rpc GetFriendList(GetFriendListRequest) returns (GetFriendListResponse);
}
```

`protoc --cpp_out=. friend.proto` 生成 friend.pb.h / friend.pb.cc。

**② 服务方(provider)**:继承生成的 Service 基类,重写方法,发布:

```cpp
#include "friend.pb.h"
#include "kopirpcapplication.h"
#include "rpcprovider.h"

class FriendService : public fixbug::FriendServiceRpc {
 public:
  void GetFriendList(::google::protobuf::RpcController*,
                     const ::fixbug::GetFriendListRequest* req,
                     ::fixbug::GetFriendListResponse* res,
                     ::google::protobuf::Closure* done) {
    res->mutable_result()->set_errcode(0);
    res->add_friends("qinwen");          // 你的业务逻辑
    done->Run();                         // 必须调:框架在此拿回响应并回发
  }
};

int main(int argc, char* argv[]) {
  KopirpcApplication::Init(argc, argv);
  RpcProvider provider;
  provider.NotifyService(new FriendService());
  provider.Run();                        // 阻塞于此
  return 0;
}
```

**③ 调用方(caller)**:

```cpp
#include "friend.pb.h"
#include "kopirpcapplication.h"
#include "kopirpcchannel.h"
#include "kopirpccontroller.h"

int main(int argc, char* argv[]) {
  KopirpcApplication::Init(argc, argv);

  fixbug::FriendServiceRpc_Stub stub(new KopiRpcChannel());
  fixbug::GetFriendListRequest request;
  request.set_userid(1000);
  fixbug::GetFriendListResponse response;
  KopirpcController controller;          // 拿回成败与错误文本

  stub.GetFriendList(&controller, &request, &response, nullptr);  // 同步阻塞

  if (controller.Failed()) {
    // 网络/框架层错误:controller.ErrorText()
  } else if (response.result().errcode() == 0) {
    // 成功:读 response.friends(i)
  }
  return 0;
}
```

## 5. 配置文件(`-i` 指定,四个 key)

```ini
rpcserverip=127.0.0.1     # provider 监听地址(前两者只有 provider 用)
rpcserverport=8000
zookeeperip=127.0.0.1     # ZK 地址(provider 注册 / caller 发现都要)
zookeeperport=2181
```

## 6. 运行前提

- ZooKeeper 已启动且可达(provider 启动时注册服务节点,连不上 ZK 会失败)
- 先起 provider(看到注册成功日志),再跑 caller

## 7. 已知限制(v0.1.0)

| 限制 | 说明 |
|---|---|
| 帧协议 | 仅 localhost 小消息场景验证;大消息/跨网络粘包半包未支持(无帧重组) |
| ZK 地址缓存 | 不过期:provider 换地址重启后,长存的 caller 进程拿不到新地址(重启 caller 即可) |
| 调用方式 | 仅同步阻塞;异步/重试/熔断未实现(见仓库 roadmap) |

## 8. 更多

- 完整可跑示例 + 压测工具:仓库 [example/](https://github.com/QinWenYan1/KopiRPC/tree/main/example)
- 性能数据与复现:[docs/benchmark.md](https://github.com/QinWenYan1/KopiRPC/blob/main/docs/benchmark.md)
- 实现原理学习笔记:仓库 [docs/](https://github.com/QinWenYan1/KopiRPC/tree/main/docs) 01 起
