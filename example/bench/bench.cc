// bench: RPC 压测客户端(阶段1 benchmark 的压测工具,单线程极简版)
// 指标: avg RT / P99 / QPS;用法: bench -i test.conf [总调用次数,默认1000]
#include <algorithm>  // sort
#include <chrono>     // steady_clock 掐表
#include <cstdlib>
#include <iostream>
#include <vector>

#include "friend.pb.h"
#include "kopirpcapplication.h"
#include "kopirpcchannel.h"
#include "kopirpccontroller.h"

int main(int argc, char* argv[]) {
  //框架初始化(从 -i 读配置文件: zookeeper ip/port 等)
  //注意: Init 只消费第一个 getopt 选项,裸参数放在 "-i conf" 之后携带是安全的
  KopirpcApplication::Init(argc, argv);

  //总调用次数: argv[3](即 -i conf 之后的第一个裸参数),默认 1000
  int total = 1000;
  if (argc >= 4) {
    total = atoi(argv[3]);
    if (total <= 0) total = 1000;
  }

  //一个 stub 反复用: 现状 channel 每次调用内部都是
  //新建 socket + ZK 查询 + 短连接收发 + 关闭,这正是阶段1要测的基线成本
  fixbug::FriendServiceRpc_Stub stub(new KopiRpcChannel());

  std::vector<double> rtts;  //成功调用的耗时(毫秒),结束排序取 avg/P99
  rtts.reserve(total);
  int ok = 0;
  int fail = 0;

  const auto wallStart = std::chrono::steady_clock::now();
  for (int i = 0; i < total; ++i) {
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
  const double wallSec =
      std::chrono::duration<double>(std::chrono::steady_clock::now() - wallStart)
          .count();

  //统计输出: 失败调用的耗时不计入(快速失败会拉低均值,造假)
  std::cout << "bench done: total=" << total << " ok=" << ok
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
