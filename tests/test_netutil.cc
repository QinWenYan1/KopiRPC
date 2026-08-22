// netutil 单元测试: SendN / RecvN 的契约验证
// 手段: socketpair(AF_UNIX) 造一对互通的本地 socket,不碰网络/ZK/框架
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "netutil.h"

namespace {

// 造一对已连接的 socket;fds[0]/fds[1] 互通
void MakePair(int fds[2]) { ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, fds)); }

void ClosePair(int fds[2]) {
  close(fds[0]);
  close(fds[1]);
}

// 小包直发直收: SendN 基本契约
TEST(NetUtil, SendNSmallPayload) {
  int fds[2];
  MakePair(fds);

  const std::string msg = "hello kopirpc";
  ASSERT_TRUE(SendN(fds[0], msg.data(), msg.size()));

  char buf[64] = {0};
  ssize_t n = read(fds[1], buf, sizeof(buf));
  ASSERT_EQ(n, static_cast<ssize_t>(msg.size()));
  EXPECT_STREQ(buf, msg.c_str());

  ClosePair(fds);
}

// 大载荷: 调小发送缓冲 + 对端边收边比,验证"全部字节按序到达"
// (阻塞 socket 下部分写入由内核与信号时机决定,无法 100% 确定性触发;
//  本用例验证的是 SendN 的契约: 要么全发完返回 true,要么 false)
TEST(NetUtil, SendNLargePayloadAllBytesInOrder) {
  int fds[2];
  MakePair(fds);

  int sndbuf = 4096;  // 调小发送缓冲,放大"一次发不完"的概率
  ASSERT_EQ(0, setsockopt(fds[0], SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)));

  const size_t total = 1 << 20;  // 1MB
  std::vector<char> payload(total);
  for (size_t i = 0; i < total; ++i) payload[i] = static_cast<char>(i * 131 + 7);

  // 接收线程: 边收边存,直到对端关写(EOF)
  std::vector<char> received;
  received.reserve(total);
  std::thread reader([&] {
    char buf[8192];
    ssize_t n;
    while ((n = read(fds[1], buf, sizeof(buf))) > 0) {
      received.insert(received.end(), buf, buf + n);
    }
  });

  ASSERT_TRUE(SendN(fds[0], payload.data(), payload.size()));
  shutdown(fds[0], SHUT_WR);  // 通知接收方:发完了
  reader.join();

  ASSERT_EQ(received.size(), total);
  EXPECT_EQ(0, memcmp(received.data(), payload.data(), total));

  ClosePair(fds);
}

// RecvN 必须读满 n 字节才返回: 对端故意分两次写
TEST(NetUtil, RecvNReadsExactAcrossSplitWrites) {
  int fds[2];
  MakePair(fds);

  std::thread writer([&] {
    write(fds[1], "abc", 3);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    write(fds[1], "def", 3);
  });

  char buf[8] = {0};
  // 第一次 write 只有 3 字节,RecvN 必须等到第二批凑满 6 才返回 true
  EXPECT_TRUE(RecvN(fds[0], buf, 6));
  EXPECT_STREQ(buf, "abcdef");

  writer.join();
  ClosePair(fds);
}

// 对端直接关闭: RecvN 读到 EOF 必须返回 false(不许死等)
TEST(NetUtil, RecvNReturnsFalseOnClosedPeer) {
  int fds[2];
  MakePair(fds);
  close(fds[1]);  // 对端立刻关闭

  char buf[16];
  EXPECT_FALSE(RecvN(fds[0], buf, sizeof(buf)));

  close(fds[0]);
}

}  // namespace
