#include "netutil.h"

#include <sys/socket.h>

#include <cerrno>

bool SendN(int fd, const char* data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = send(fd, data + sent, len - sent, MSG_NOSIGNAL);
    if (n > 0) {
      sent += static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR)
      continue;  // EINTR 被信号打断, 不一定代表socket坏了，重试
    return false;  //真错误(含对端关闭)
  }
  return true;
}

bool RecvN(int fd, char* buf, size_t len) {
  size_t got = 0;
  while (got < len) {
    ssize_t n = recv(fd, buf + got, len - got, 0);
    if (n > 0) {
      got += static_cast<size_t>(n);
      continue;
    }
    if (n == -1 && errno == EINTR) continue;
    return false;  // n==0 = 对端关闭;n==-1 = 错误
  }
  return true;
}