// KopiConnectPool 单元测试: 借/还/焚毁语义
// 手段: 本地起 listen socket 当假服务器,用 accept 计数观察"是否真的复用了连接"
// (不断言 fd 数值 —— OS 会回收 fd 号,复用与否要看"有没有新建连接",即 accept
// 计数)
#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <thread>

#include "kopiconnectpool.h"

namespace {

// 假服务器: 只 listen + accept 计数,不读不写
// (connect 成功不需要对端 accept,内核 backlog
// 会接住握手;计数只为观察连接新建次数)
struct FakeServer {
  int lfd = -1;
  uint16_t port = 0;
  std::atomic<int> accepted{0};
  std::thread acceptThread;

  FakeServer() {
    lfd = socket(AF_INET, SOCK_STREAM, 0);
    EXPECT_GE(lfd, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = 0;  // 让内核挑空闲端口
    EXPECT_EQ(0, bind(lfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)));
    EXPECT_EQ(0, listen(lfd, 8));
    socklen_t len = sizeof(addr);
    EXPECT_EQ(0, getsockname(lfd, reinterpret_cast<sockaddr*>(&addr), &len));
    port = ntohs(addr.sin_port);

    acceptThread = std::thread([this] {
      while (accept(lfd, nullptr, nullptr) >= 0) {
        ++accepted;
      }  // close(lfd) 后 accept 返回 -1,线程退出
    });
  }
  ~FakeServer() {
    close(lfd);
    acceptThread.join();
  }
};

}  // namespace

// 借用: 能借到一个合法已连接的 fd
TEST(ConnectPool, BorrowNewConnectionWorks) {
  FakeServer srv;
  auto& pool = KopiConnectPool::GetInstance();

  int fd = pool.BorrowConnection("127.0.0.1", srv.port);
  ASSERT_GE(fd, 0);

  pool.ReturnConnection("127.0.0.1", srv.port, fd, false);
}

// 健康归还 → 再借 = 复用(没有新建连接)
TEST(ConnectPool, ReturnedConnectionIsReused) {
  FakeServer srv;
  auto& pool = KopiConnectPool::GetInstance();

  int fd1 = pool.BorrowConnection("127.0.0.1", srv.port);
  ASSERT_GE(fd1, 0);
  pool.ReturnConnection("127.0.0.1", srv.port, fd1, false);  // 健康归还

  int fd2 = pool.BorrowConnection("127.0.0.1", srv.port);
  ASSERT_GE(fd2, 0);
  pool.ReturnConnection("127.0.0.1", srv.port, fd2, false);

  // 复用的铁证: 假服务器只 accept 到 1 次(没有第二次握手)
  EXPECT_EQ(1, srv.accepted.load());
}

// is_bad 归还 → 焚毁 → 再借 = 新建(accept 计数 +1)
TEST(ConnectPool, BadConnectionIsBurnedNotReused) {
  FakeServer srv;
  auto& pool = KopiConnectPool::GetInstance();

  int fd1 = pool.BorrowConnection("127.0.0.1", srv.port);
  ASSERT_GE(fd1, 0);
  pool.ReturnConnection("127.0.0.1", srv.port, fd1, true);  // is_bad: 焚毁

  int fd2 = pool.BorrowConnection("127.0.0.1", srv.port);
  ASSERT_GE(fd2, 0);
  pool.ReturnConnection("127.0.0.1", srv.port, fd2, false);

  // 焚毁的铁证: 假服务器 accept 到 2 次(第二次是新建的)
  EXPECT_EQ(2, srv.accepted.load());
}
