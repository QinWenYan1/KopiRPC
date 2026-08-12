#include "zookeeperutil.h"

#include <semaphore.h>
#include <zookeeper/zookeeper.h>

#include <iostream>

#include "kopirpcapplication.h"  //需要ip和端口号，所有信息都在这个文件

ZkClient::ZkClient() : mZhandle(nullptr) {}

ZkClient::~ZkClient() {
    //关闭句柄，释放资源
  if (mZhandle) zookeeper_close(mZhandle); 
}

//链接zkserver 
void ZkClient::Start(){
    std::string host = KopirpcApplication::GetInstance().GetConfigFile().Load("zookeeperip"); 
    std::string port = KopirpcApplication::GetInstance().GetConfigFile().Load("zookeeperport");
}