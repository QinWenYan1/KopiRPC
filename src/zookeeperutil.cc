#include "zookeeperutil.h"

#include <google/protobuf/stubs/strutil.h>
#include <semaphore.h>
#include <zookeeper/zookeeper.h>

#include <cstdlib>
#include <iostream>

#include "kopirpcapplication.h"  //需要ip和端口号，所有信息都在这个文件
#include "logger.h"

ZkClient::ZkClient() : mZhandle(nullptr) {}

ZkClient::~ZkClient() {
  //关闭句柄，释放资源
  if (mZhandle) zookeeper_close(mZhandle);
}

//全局watcher观察器，zkserver给zkclient的通知
void globalWatcher(zhandle_t* zh, int type, int state, const char* path,
                   void* watcherCtx) {
  if (type == ZOO_SESSION_EVENT) {  //回调的消息类型是和会话相关的消息类型
    if (state == ZOO_CONNECTED_STATE) {  // zk client和zkserver链接成功
      //获取信号量
      sem_t* sem = static_cast<sem_t*>(const_cast<void*>(zoo_get_context(zh)));
      //信号量资源+1
      sem_post(sem);
    }
  }
}

//链接zkserver
void ZkClient::Start() {
  std::string host =
      KopirpcApplication::GetInstance().GetConfigFile().Load("zookeeperip");
  std::string port =
      KopirpcApplication::GetInstance().GetConfigFile().Load("zookeeperport");
  std::string connStr = host + ":" + port;

  // 初始化句柄
  // 但是并不是句柄返回就代表链接成功，zookeeper_mt 是异步多线程的
  // 其提供三个线程:
  //   1. 一个线程是zookeeper_init
  //   2. 一个线程是网络I/O使用pthread实现poll
  //   3. 当客户端接受到server响应的时候，一个线程启动watcher回调
  mZhandle = zookeeper_init(connStr.c_str(), globalWatcher, 30000, nullptr,
                            nullptr, 0);
  if (!mZhandle) {
    LOG_ERR("zookeeper_init error!");
    exit(EXIT_FAILURE);
  }
  //创建信号量,等待句柄完成回调
  sem_t sem;
  sem_init(&sem, 0, 0);
  zoo_set_context(mZhandle, &sem);

  sem_wait(&sem);
  std::cout << "zookeeper_init success" << std::endl;
  LOG_INFO("zookeeper_init success");
}

void ZkClient::Create(const char* path, const char* data, int datalen,
                      int state) {
  std::string pathStr = static_cast<std::string>(path);
  int StrSize = pathStr.size();
  int flag;
  //先判断path表示的znode节点是否存在，如果存在，就不在重复创建了
  flag = zoo_exists(mZhandle, pathStr.c_str(), 0, nullptr);
}