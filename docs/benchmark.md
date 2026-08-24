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

## bench 代码逻辑(怎么测的)

源码:[example/bench/bench.cc](../example/bench/bench.cc)(~110 行)

**一句话:同步循环打 N 次 `GetFriendList`,每次掐一次表,最后排序统计。**

**流程:**

- **main**
  - `KopirpcApplication::Init(-i test.conf)` → 读 ZK 地址等配置
  - 解析裸参数: `argv[3]`=每线程次数(默认 1000),`argv[4]`=线程数(默认 1)
  - 起 N 个线程各跑一份 `BenchWorker`,join 等全部结束
  - 合并 N 份结果 → 排序 → 输出 avg / P99 / QPS

- **BenchWorker(每线程一份)**
  - `new KopiRpcChannel()` 造**本线程私有** stub(Stub 不可跨线程共享)
  - 循环"每线程次数"次:
    - 造 request / response / controller
    - `t0 = now` → `stub.GetFriendList(...)`(同步阻塞,一次完整 RPC 往返)→ `t1 = now`
    - 成功 → 耗时 (t1-t0)ms 入本线程 vector;失败 → fail++(前 3 次打印原因)
  - 全程无锁: 计数与耗时都是线程私有的,join 后才合并

**四个刻意的设计决策:**

| 决策 | 为什么 |
|---|---|
| 掐表只掐 `stub.GetFriendList` 一行 | 测的是 RPC 往返,不是 request 构造 |
| 失败调用不计入耗时 | 快速失败会拉低均值 = 造假 |
| QPS = 总 ok ÷ wall 时间(不是 avg 倒数) | 并发下 avg 倒数 ≠ 真实吞吐;wall 掐在 spawn 前→join 完 |
| P99 = 排序后 99% 位的那个样本 | 1000 次取第 990 名,简单直接 |

## 复现步骤(手把手)

> ⚠️ release 包含库 + 头文件 + USAGE 文档,**不含 example/**;复现请用 git 仓库源码。

**0. 环境(一次性)**

全部依赖都在 Docker 镜像里,宿主机零安装:

```bash
docker pull cspenguin/qwenrpc-dev:latest
git clone https://github.com/QinWenYan1/KopiRPC.git
cd KopiRPC
docker run -it --rm -v "$(pwd)":/workspace -w /workspace cspenguin/qwenrpc-dev:latest
```

之后所有命令都在容器内执行。

**1. 构建**

```bash
./autobuild.sh
```

> `provider` / `bench` 属于 example,随仓库构建自动产出到 `bin/`,无需额外开关。

**2. 起 ZooKeeper**

```bash
service zookeeper start    # 若提示无此服务: zkServer.sh start;默认监听 2181
```

**3. 写 test.conf(本地文件,不入库)**

项目根目录新建 `test.conf`,四行:

```ini
rpcserverip=127.0.0.1
rpcserverport=8000
zookeeperip=127.0.0.1
zookeeperport=2181
```

> provider 读前两行决定监听地址;caller/bench 读后两行去 ZK 发现服务。

**4. 起 provider(新开一个终端进同一容器)**

```bash
docker exec -it <容器名或ID> bash
./bin/provider -i test.conf
```

看到 `znode create successfully... /FriendServiceRpc/GetFriendList` 即就绪。

**5. 跑 bench(回到第一个终端)**

```bash
./bin/bench -i test.conf 1000      # 单线程 1000 次 —— 与基线①同参数
./bin/bench -i test.conf 1000 8    # 8 线程 × 1000
```

预期输出(数字随机波动,量级一致即可):

```
bench done: threads=1 total=1000 ok=1000 fail=0
avg RT: ~0.08 ms
P99 RT: ~0.17 ms
QPS: ~12000
```

**顺带可看**:provider 日志里 `newConnection` 条数 —— 连接池复用时,
单线程 1000 次调用只有 ~1 条(优化前是 1000 条)。

---