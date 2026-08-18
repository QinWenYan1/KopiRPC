// bench: RPC 压测客户端(阶段1 benchmark 的压测工具)
// Step 1 骨架: 单线程循环调用 GetFriendList N 次,统计成功/失败
// 用法: bench -i test.conf [总调用次数,默认1000]
#include <cstdlib>
#include <iostream>

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

  int ok = 0;
  int fail = 0;
  for (int i = 0; i < total; ++i) {
    fixbug::GetFriendListRequest request;
    request.set_userid(2000);
    fixbug::GetFriendListResponse response;
    KopirpcController controller;  //每次调用配一个: 拿回成败与错误文本

    //同步阻塞调用,底层走 KopiRpcChannel::CallMethod
    stub.GetFriendList(&controller, &request, &response, nullptr);

    if (!controller.Failed() && response.result().errcode() == 0) {
      ++ok;
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

  std::cout << "bench done: total=" << total << " ok=" << ok
            << " fail=" << fail << std::endl;
  return 0;
}
