#pragma once

#include <semaphore.h>
#include <zookeeper/zookeeper.h>

#include <string>

//封装的zk客户端类
class ZkClient {
 public:
  ZkClient() = default;
  ~ZkClient();
  // zkclient启动链接zkserver
  void Start();
  //在zkserver上根据指定的path创建znode节点
  // 默认state为0就是永久性节点
  void Create(const char* path, const char* data, int datalen, int state = 0);
  //根据指定znode
  std::string GetData(const char* path);

 private:
  // zk的客户端句柄，通过句柄来操作zk server
  zhandle_t* mZhandle;
};