#include "lockqueue.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

// 被测契约(见 lockqueue.h):多生产者线程安全 Push;单消费者 Pop;
// 队列空时 Pop 阻塞等待,Push 后 notify_one 唤醒

// ① 单线程先进先出: push 1..100,pop 必须原序出来
TEST(LockQueueTest, SingleThreadFIFO) {
  LockQueue<int> q;
  for (int i = 1; i <= 100; ++i) q.Push(i);
  for (int i = 1; i <= 100; ++i) {
    EXPECT_EQ(q.Pop(), i);
  }
}

// ② 并发正确性: 4 个生产者各发 1000 条(号段互不重叠),
//   消费者收齐 4000 条,排序后必须恰好是 1..4000 —— 丢一条/重一条都会暴露
TEST(LockQueueTest, MultiProducerNoLossNoDup) {
  LockQueue<int> q;
  constexpr int kProducers = 4;
  constexpr int kPerProducer = 1000;
  constexpr int kTotal = kProducers * kPerProducer;

  std::vector<std::thread> producers;
  for (int p = 0; p < kProducers; ++p) {
    producers.emplace_back([&q, p] {
      for (int i = 1; i <= kPerProducer; ++i) q.Push(p * kPerProducer + i);
    });
  }

  std::vector<int> received;
  received.reserve(kTotal);
  for (int i = 0; i < kTotal; ++i) received.push_back(q.Pop());
  for (auto& t : producers) t.join();

  std::sort(received.begin(), received.end());
  for (int i = 0; i < kTotal; ++i) {
    EXPECT_EQ(received[i], i + 1);
  }
}

// ③ 空队列时 Pop 必须阻塞(不是忙等/返回垃圾),Push 后被唤醒拿到值
TEST(LockQueueTest, PopBlocksUntilPush) {
  LockQueue<int> q;
  std::atomic<bool> popped{false};

  std::thread consumer([&] {
    int v = q.Pop();
    EXPECT_EQ(v, 42);
    popped = true;
  });

  // 给消费者充分时间进入 wait;期间它必须仍处于阻塞状态
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(popped.load());

  q.Push(42);
  consumer.join();
  EXPECT_TRUE(popped.load());
}
