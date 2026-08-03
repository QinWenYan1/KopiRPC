#pragma once
#include <condition_variable>  //pthread_condition_t
#include <mutex>               //pthread_mutex_t
#include <queue>
#include <thread>

//异步写日志的日志队列
template <typename T>
class LockQueue {
 public:
    void Push(const T& data); 
    T&  Pop(); 
 private:   
    std::queue<T> q; 
    std::mutex mtx; 
    std::condition_variable conVar; 
};