#include <gtest/gtest.h>

#include <cstdio>
#include <unistd.h>

// 自写 main 而不用 gtest_main 的原因:
//   Logger 的分离写日志线程是 for(;;) 死循环且没有停止机制,
//   正常 return/exit 会在析构静态 Logger 时卡死(condvar 上还睡着等待者,未定义行为)。
//   测试程序跑完即弃,直接 _exit 跳过静态析构;fflush 先把 gtest 输出冲干净。
int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  std::fflush(nullptr);
  _exit(ret);  // POSIX: 立即终止进程,不走 atexit/静态析构
}
