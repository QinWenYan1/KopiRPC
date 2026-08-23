# KopiRPC Benchmark 报告

> 压测工具:[example/bench/](../example/bench/bench.cc)(支持线程参数)
> 方法:同步循环调用 `FriendServiceRpc.GetFriendList`,每次 `steady_clock` 掐表,结束排序统计;失败调用不计入耗时统计。多线程时每线程独立 stub 与计数,join 后合并;QPS = 总 ok / wall 时间。

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

## 数字②:阶段2 优化后(2026-08-23)

**优化内容**:连接池(连接跨调用复用)+ ZK 地址缓存(命中跳过整个 ZK 会话)
+ 响应长度头(池化的硬前置)+ SendN/RecvN(全发全收)。
> bench 已支持多线程:`bench -i test.conf [每线程次数] [线程数]`。

| 场景 | ok / fail | avg RT | P99 RT | QPS |
|---|---|---|---|---|
| 单线程 1000 | 1000 / 0 | **0.083 ms** | **0.166 ms** | **11976** |
| 8 线程 ×1000 | 8000 / 0 | 0.134 ms | 0.340 ms | **58019** |

**对比基线①**(单线程同法):avg RT 1.62→0.083ms(**~20×**),P99 2.64→0.166ms(~16×),QPS 615→11976(**~19×**);8 线程吞吐 58019 QPS。

**复用证据**:caller 侧每次调用的 ZK 会话消失 —— 8 线程日志可见 ZK session 恰好 8 条(冷启动 miss 各查一次,之后全走缓存);单线程仅 1 条。

**解读**:每调用固定开销(1 次 ZK 会话 + 1 次 TCP 建/断连 ≈ 1.5ms)被砍掉后,localhost 单发的真实成本浮出:微秒级。8 线程 avg RT 略升(0.083→0.134ms)是并发下排队与日志锁的自然代价 —— 用延迟换吞吐,正常。
