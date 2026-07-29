#include "kopirpcapplication.h"

#include <unistd.h>

#include <iostream>
#include <string>

#include "kopirpcconfig.h"

void KopirpcApplication::ShowArgsHelp() {
  std::cout << "format: command -i <configfile>" << std::endl;
}

// 静态成员的类外定义: 类内只是声明,必须且只能在类外定义一次
KopirpcConfig KopirpcApplication::config;

// Init 三段式: 校验参数 -> getopt 解析 -> 加载并回显配置
void KopirpcApplication::Init(int argc, char** argv) {
  if (argc < 2)  // 没传任何参数,直接提示用法并退出
  {
    ShowArgsHelp();
    exit(EXIT_FAILURE);
  }

  int c = 0;
  std::string config_file;
  // getopt 逐个消费选项; "i:" 表示 -i 必须带一个参数(即配置文件路径)
  if ((c = getopt(argc, argv, "i:")) != -1) {
    switch (c) {
      case 'i':
        config_file = optarg;
        break;
      case '?':  // 未知选项,或 -i 缺参数(optstring 无 ':' 前缀时缺参数也走这里)
        ShowArgsHelp();
        exit(EXIT_FAILURE);
      case ':':  // 仅当 optstring 以 ':' 开头时才返回;当前不会触发,防御保留
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }
  } else {  //当 输入 ./provider test.conf 时也会有错误提示
    ShowArgsHelp();
    exit(EXIT_FAILURE);
  }

  /*开始加载配置文件(约定配置项见 KopirpcConfig 头文件注释)*/
  config.LoadConfigFile(config_file.c_str());
  // 回显加载结果,启动时便于人工确认配置正确
  std::cout << "RPC Server ip:" << config.Load("rpcserverip") << std::endl;
  std::cout << "RPC Server port:" << config.Load("rpcserverport") << std::endl;
  std::cout << "Zookeeper ip:" << config.Load("zookeeperip") << std::endl;
  std::cout << "Zookeeper port:" << config.Load("zookeeperport") << std::endl;
}

// 函数内静态对象: 首次调用时构造,之后永远返回同一实例(C++11 起构造线程安全)
KopirpcApplication& KopirpcApplication::GetInstance() {
  static KopirpcApplication app;
  return app;
}

//返回配置文件
const KopirpcConfig& KopirpcApplication::GetConfigFile() { return config; }
