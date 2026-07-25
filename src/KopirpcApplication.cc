#include "KopirpcApplication.h"
#include <unistd.h>
#include <iostream>
#include <string>

inline void ShowArgsHelp() {
  std::cout << "format: command -i <configfile>" << std::endl;
}

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
        std::cout << "invalid option args!" << std::endl;
        ShowArgsHelp();
        exit(EXIT_FAILURE);
      case ':':
        std::cout << "need config file!" << std::endl;
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
  }

  /*
   * 开始加载配置文件了
   */

  /*
   * 配置文件内容：
   * rpcserver_ip, rpcserver_port
   * zookeeper_ip, zookeeper_port
   */
}

KopirpcApplication& KopirpcApplication::GetInstance() {
  static KopirpcApplication app;
  return app;
}