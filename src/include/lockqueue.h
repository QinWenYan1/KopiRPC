#pragma once
#include <condition_variable>  //pthread_condition_t
#include <mutex>               //pthread_mutex_t
#include <queue>
#include <thread>

//异步写日志的日志队列
template <typename T>
class LockQueue {
 public:
  //多个worker线程都会写日志queue
  void Push(const T& data) {
    //加锁
    std::lock_guard<std::mutex> lock(mtx);
    //临界区
    q.push(data);
    //锁自动释放,由于写日志线程只有一个就只需要notify one
    conVar.notify_one();
  }

  //只有一个线程会阅读日志queue
  T Pop() {
    std::unique_lock<std::mutex> lock(mtx);
    while (q.empty()) {
      //日志队列为空，线程进入wait状态
      //使用while避免假唤醒
      conVar.wait(lock);
    }
    T data = q.front();
    q.pop();
    return data;
  }

 private:
  std::queue<T> q;
  std::mutex mtx;
  std::condition_variable conVar;
};