#include "logger.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>  // LOG_INFO 宏展开后用到 snprintf,使用方必须自己带上
#include <ctime>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

// 被测行为(见 logger.cc): Logger 构造即起分离写日志线程,
// 循环「打开当天 YYYY-M-D-log.txt -> Pop 阻塞等 -> 写入 时:分:秒 => [级别]消息」
// 副作用: 测试运行目录会落一个当天的日志文件(logger 本来就是这样)

// 与 logger.cc 相同的文件名规则: 年-月-日-log.txt(月日不补零)
static std::string DailyLogName() {
  time_t now = time(nullptr);
  tm* t = localtime(&now);
  return std::to_string(t->tm_year + 1900) + "-" +
         std::to_string(t->tm_mon + 1) + "-" + std::to_string(t->tm_mday) +
         "-log.txt";
}

// 读当天日志全文;文件还没被创建时返回空串
static std::string ReadDailyLog() {
  std::ifstream in(DailyLogName());
  if (!in.is_open()) return "";
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// 统计 content 里 needle 出现的次数
static int CountOccurrences(const std::string& content,
                            const std::string& needle) {
  int cnt = 0;
  size_t pos = 0;
  while ((pos = content.find(needle, pos)) != std::string::npos) {
    ++cnt;
    ++pos;  // 前进一格,允许重叠匹配
  }
  return cnt;
}

// 轮询直到条件满足(每 50ms 查一次,至多 2s)——不写死 sleep,不怕慢机器抖动
template <typename Pred>
static bool PollUntil(Pred pred) {
  for (int i = 0; i < 40; ++i) {
    if (pred()) return true;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return false;
}

// ① 单例: 两次获取必须是同一个对象(全局唯一日志器)
TEST(LoggerTest, SingletonSameInstance) {
  EXPECT_EQ(&Logger::GetInstance(), &Logger::GetInstance());
}

// ② 全链路: LOG_INFO 宏格式化 -> 进队列 -> 写日志线程落盘,带 [info] 标签
//   logger.cc 的格式是 "...=> [info]" 后紧跟消息体,中间无空格
TEST(LoggerTest, InfoLogWrittenToDailyFile) {
  LOG_INFO("kopirpc test info %d", 42);
  EXPECT_TRUE(PollUntil([] {
    return ReadDailyLog().find("[info]kopirpc test info 42") !=
           std::string::npos;
  }));
}

// ③ 级别标签: LOG_ERR 落盘必须带 [error]
TEST(LoggerTest, ErrLogTaggedError) {
  LOG_ERR("kopirpc test err");
  EXPECT_TRUE(PollUntil([] {
    return ReadDailyLog().find("[error]kopirpc test err") != std::string::npos;
  }));
}

// ④ 并发冒烟: 4 线程 × 50 条,不崩不卡;
//   且本用例新增的 200 条最终全部落盘(=队列全消费,顺手再压一遍 LockQueue)
//   注意: 日志文件是追加模式且跨运行存活,断言必须用"本用例新增条数"而非总条数,
//   否则从第二次运行起必挂(基数里躺着上一次运行的 200 条)
TEST(LoggerTest, ConcurrentLogNoCrash) {
  constexpr int kThreads = 4;
  constexpr int kPerThread = 50;
  const int kTotal = kThreads * kPerThread;

  const int before = CountOccurrences(ReadDailyLog(), "kopirpc mt");

  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([t, kPerThread] {
      for (int i = 0; i < kPerThread; ++i) LOG_INFO("kopirpc mt %d-%d", t, i);
    });
  }
  for (auto& th : threads) th.join();

  EXPECT_TRUE(PollUntil([before, kTotal] {
    return CountOccurrences(ReadDailyLog(), "kopirpc mt") - before == kTotal;
  }));
}
