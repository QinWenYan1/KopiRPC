![KopiRPC](docs/images/cover.jpeg)

>KopiRPC (Kopi Remote Procedure Call) —— C++ 高性能远程程序调用服务框架

![License](https://img.shields.io/github/license/QinWenYan1/KopiRPC) ![C++17](https://img.shields.io/badge/dialect-C%2B%2B17-blue) ![Platform](https://img.shields.io/badge/platform-Linux%20arm64-orange) ![Version](https://img.shields.io/github/v/release/QinWenYan1/KopiRPC)

---

## 📖 什么是 RPC？

- `RPC (Remote Procedure Call, 远程过程调用)` 是一种通信技术：让程序**像调用本地函数一样，调用另一台机器上的函数**，而无需关心底层的网络细节。

    ```cpp
    // 本地调用：函数就在当前进程里
    bool ok = Login("zhang san", "123456");

    // RPC 调用：函数实际运行在远端服务器上，但写法几乎一样
    stub.Login(nullptr, &request, &response, nullptr);
    ```

- 一句话总结：**RPC 的本质 = 函数调用的语义 + 网络传输的实现**。

---
## 🤔 为什么需要 RPC？


**1. 简化远程调用的复杂性**

- 在分布式系统中，服务往往部署在不同的机器或网络中。如果没有 RPC，每一次跨机调用，开发人员都需要手动编写复杂的网络通信代码：
    - **创建和管理连接**
    - **数据的序列化和反序列化**
    - **请求和响应的发送与接收**
    - **错误处理（如超时、断线重连等）**

- RPC 对上述底层逻辑进行了统一封装，开发人员只需专注于业务逻辑，无需关心底层的通信细节。

**2. 提升开发效率**

- **抽象通信过程**：只需定义远程调用的接口（如方法签名），无需实现复杂的通信逻辑
- **统一接口定义**：主流 RPC 框架（如 gRPC）提供 IDL（接口描述语言），只需编写一份接口定义，框架即可自动生成客户端和服务端代码
- **减少出错概率**：复杂的通信逻辑由框架封装，避免手写网络代码时的常见错误（如序列化失败、超时处理不当等）

--- 
## 🎯 RPC 的应用场景

- RPC 不仅用于微服务，还广泛应用于以下场景：

    1. **分布式系统** —— 解决跨网络的通信问题，提供一致的调用体验
    2. **跨语言服务调用** —— 如 gRPC 支持 C++、Java、Python 等多语言，实现异构服务之间的互相调用
    3. **高性能内部通信** —— 利用高性能序列化协议和连接复用能力，减少网络通信开销
    4. **服务间依赖调用** —— 如电商系统中，订单服务调用库存服务、库存服务调用支付服务，RPC 让服务间协作高效可靠

--- 

## 📚 学习文档

- 想揭开 RPC 的奥秘，推荐参考以下学习文档：
    - [01 - C++实现轻量级RPC分布式网络通信框架（ZooKeeper + ProtoBuf + Muduo）](docs/01-RPC框架实现-ZooKeeper-ProtoBuf-Muduo.md)
    - [02 - Protocol Buffers 概览](docs/02-Protocol-Buffers-概览.md)
    - [03 - Protocol Buffers C++ 教程](docs/03-Protocol-Buffers-C++教程.md)

- 前置网络知识补充：
    - [高性能网络编程基础知识（I/O 多路复用，poll/select/epoll，Reactor/Proactor）](https://github.com/QinWenYan1/Network_System/tree/main/Chapter02_Application_Layer)


---

## 🐳 Docker 开发环境

本项目提供一键可用的 Docker 开发镜像，预装了 C++ 工具链、Protobuf、Muduo、ZooKeeper 等全部依赖。

- 详细说明：[docs/docker-dev-setup.md](docs/docker-dev-setup.md)
- 快速开始：

  ```bash
  docker pull cspenguin/qwenrpc-dev:latest
  docker run -it --rm -v "$(pwd)":/workspace -w /workspace cspenguin/qwenrpc-dev:latest
  ```
  
---

## 🗺️ Roadmap

> 采用「完成即划掉」的待办清单风格，随项目推进持续更新。

- ~~[x] 配置容器化开发环境（Docker + Muduo + ZooKeeper）~~
- ~~[x] 搭建编译逻辑与环境（CMake 构建体系 + clangd）~~
- ~~[x] 搭建 RPC 基本框架~~
- ~~[x] 实现 RPC 服务发布（基于 protobuf 反射的服务/方法注册）~~
- ~~[x] 完成 provider 请求处理（拆包解析 → 方法分发 → 响应回发）~~
- ~~[x] 实现 caller 调用通道（RpcChannel:stub 像本地调用一样发起远程请求）~~
- ~~[x] 实现日志系统（异步队列 + 写日志线程，框架打印全面接入）~~
- ~~[x] 接入 ZooKeeper 服务注册与发现~~
- ~~[x] 编写自动编译脚本(autobuild.sh:一键完成 cmake 配置与构建)~~
- ~~[x] 端到端联调(provider + caller 跑通示例服务)~~
- [ ] P1 单元测试基建(gtest + config/LockQueue 纯逻辑测试)
- [ ] P2 正确性补强(帧重组 + 响应长度头 + send 全发循环 + 配套测试)
- [ ] P3 benchmark 基线(压测客户端:QPS + RT 分位数)
- [ ] P4 性能优化(长连接 + ZK 地址缓存,前后对比)
- [ ] P5 出包发布(v0.1.0:tar.gz + 使用文档 + GitHub Release)
- [ ] P6 消费验证(外部项目以包形式调通 toy 服务)
- [ ] 进阶特性(远期):异步调用、负载均衡、重试、健康检查、熔断


