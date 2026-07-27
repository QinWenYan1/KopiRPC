#include "kopirpcapplication.h"
#include <unistd.h>
#include <iostream>
#include <string>
#include "kopirpcconfig.h"

//私人工具函数：用户提示
void KopirpcApplication::ShowArgsHelp() {
  std::cout << "format: command -i <configfile>" << std::endl;
}

KopirpcConfig KopirpcApplication::config;  //类外初始化静态成员

void KopirpcApplication::Init(int argc, char** argv) {
  if (argc < 2)  // user没有输入参数，不允许，报错
  {
    ShowArgsHelp();
    exit(EXIT_FAILURE);
  }

  int c = 0;
  std::string config_file;
  while ((c = getopt(argc, argv, "i:")) != -1) {
    switch (c) {
      case 'i':
        config_file = optarg;
        break;
      case '?':
        ShowArgsHelp();
        exit(EXIT_FAILURE);
      case ':':
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
  }

  /*
   * 开始加载配置文件了
   * 配置文件内容：
   * rpcserver_ip, rpcserver_port
   * zookeeper_ip, zookeeper_port
   */
  config.LoadConfigFile(config_file.c_str());
  std::cout << "RPC Server ip:" << config.Load("rpcserverip") << std::endl;
  std::cout << "RPC Server port:" << config.Load("rpcserverport") << std::endl;
  std::cout << "Zookeeper ip:" << config.Load("zookeeperip") << std::endl;
  std::cout << "Zookeeper port:" << config.Load("zookeeperport") << std::endl;
}

KopirpcApplication& KopirpcApplication::GetInstance() {
  static KopirpcApplication app;
  return app;
}

//返回配置文件
const KopirpcConfig& KopirpcApplication::GetConfigFile() { return config; }