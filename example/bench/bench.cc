// bench: RPC 压测客户端(阶段2: 支持多线程)
// 指标: avg RT / P99 / QPS
// 用法: bench -i test.conf [每线程次数,默认1000] [线程数,默认1]
#include <algorithm>  // sort
#include <chrono>     // steady_clock 掐表
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include "friend.pb.h"
#include "kopirpcapplication.h"
#include "kopirpcchannel.h"
#include "kopirpccontroller.h"

// 单线程压测主体: 每线程独立 stub(Stub 不可跨线程共享)、独立计数与耗时,
// 全程无锁 —— 主线程 join 后统一合并
static void BenchWorker(int perThread, int& ok, int& fail,
                        std::vector<double>& rtts) {
  fixbug::FriendServiceRpc_Stub stub(new KopiRpcChannel());
  rtts.reserve(perThread);

  for (int i = 0; i < perThread; ++i) {
    fixbug::GetFriendListRequest request;
    request.set_userid(2000);
    fixbug::GetFriendListResponse response;
    KopirpcController controller;  //每次调用配一个: 拿回成败与错误文本

    //同步阻塞调用,底层走 KopiRpcChannel::CallMethod;掐表只掐这一次往返
    const auto t0 = std::chrono::steady_clock::now();
    stub.GetFriendList(&controller, &request, &response, nullptr);
    const auto t1 = std::chrono::steady_clock::now();

    if (!controller.Failed() && response.result().errcode() == 0) {
      ++ok;
      rtts.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    } else {
      ++fail;
      //只打印前几次失败原因,避免刷屏
      if (fail <= 3) {
        std::cout << "call failed: "
                  << (controller.Failed() ? controller.ErrorText()
                                          : response.result().errmsg())
                  << std::endl;
      }
    }
  }
}

int main(int argc, char* argv[]) {
  //框架初始化(从 -i 读配置文件: zookeeper ip/port 等)
  //注意: Init 只消费第一个 getopt 选项,裸参数放在 "-i conf" 之后携带是安全的
  KopirpcApplication::Init(argc, argv);

  //每线程调用次数: argv[3](即 -i conf 之后的第一个裸参数),默认 1000
  int perThread = 1000;
  if (argc >= 4) {
    perThread = atoi(argv[3]);
    if (perThread <= 0) perThread = 1000;
  }
  //线程数: argv[4],默认 1
  int threads = 1;
  if (argc >= 5) {
    threads = atoi(argv[4]);
    if (threads <= 0) threads = 1;
  }

  //连接池时代: 同一进程内连接跨调用复用,N 线程预期只在 provider 侧留下
  //~N 条 newConnection(基线短连接时代是"调用总数"条) —— 复用铁证
  std::vector<std::thread> workers;
  std::vector<int> oks(threads, 0), fails(threads, 0);
  std::vector<std::vector<double>> rttsPerThread(threads);

  const auto wallStart = std::chrono::steady_clock::now();
  for (int i = 0; i < threads; ++i) {
    workers.emplace_back(BenchWorker, perThread, std::ref(oks[i]),
                         std::ref(fails[i]), std::ref(rttsPerThread[i]));
  }
  for (auto& w : workers) w.join();
  const double wallSec =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - wallStart)
          .count();

  //合并各线程结果
  int ok = 0, fail = 0;
  std::vector<double> rtts;
  rtts.reserve(static_cast<size_t>(perThread) * threads);
  for (int i = 0; i < threads; ++i) {
    ok += oks[i];
    fail += fails[i];
    rtts.insert(rtts.end(), rttsPerThread[i].begin(), rttsPerThread[i].end());
  }

  //统计输出: 失败调用的耗时不计入(快速失败会拉低均值,造假)
  std::cout << "bench done: threads=" << threads
            << " total=" << perThread * threads << " ok=" << ok
            << " fail=" << fail << std::endl;
  if (ok > 0) {
    std::sort(rtts.begin(), rtts.end());
    double sum = 0;
    for (double r : rtts) sum += r;
    const double avg = sum / rtts.size();
    //排序后取 99% 位置(如 1000 次取第 990 名)
    const double p99 = rtts[static_cast<size_t>((rtts.size() - 1) * 0.99)];
    std::cout << "avg RT: " << avg << " ms" << std::endl;
    std::cout << "P99 RT: " << p99 << " ms" << std::endl;
    std::cout << "QPS: " << ok / wallSec << std::endl;
  }
  return 0;
}
