# KopiRPC Benchmark 报告

> 压测工具:[example/bench/](../example/bench/bench.cc)(单线程极简版)
> 方法:单线程同步循环调用 `FriendServiceRpc.GetFriendList`,每次 `steady_clock` 掐表,结束排序统计;失败调用不计入耗时统计。

## 环境

| 项 | 值 |
|---|---|
| 运行环境 | Docker 容器 `cspenguin/qwenrpc-dev`(Ubuntu 22.04,linux/arm64 @ Apple Silicon) |
| 编译器 | GCC 11.4 |
| 依赖 | protobuf 3.12.4 / muduo / zookeeperd |
| 网络 | 容器内 localhost(bench、provider、ZK 同机) |

## 基线①:优化前(2026-08-19)

**成本模型**:每次 RPC 调用 = 1 次 ZooKeeper 会话(连接 + 查询)+ 1 次 TCP 短连接(建连 → 收发 → 关连)。无缓存、无复用。

| 调用次数 | ok / fail | avg RT | P99 RT | QPS |
|---|---|---|---|---|
| 100 | 100 / 0 | 1.80 ms | 4.30 ms | 556 |
| 1000 | 1000 / 0 | **1.62 ms** | **2.64 ms** | **615** |

**解读**:localhost 真实传输是微秒级,avg 1.62ms 几乎全是每调用的 ZK 会话与 TCP 建/断连开销——这是阶段2(长连接 + ZK 地址缓存)要砍掉的部分。

## 数字②:阶段2 优化后

待补(同一 bench 复测:单线程同法对比 + 追加 8 线程档)。
