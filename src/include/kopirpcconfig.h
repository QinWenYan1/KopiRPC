#pragma once
#include <string>
#include <unordered_map>

// 框架读取配置文件类
// rpcserverip rpcserverport zookeeperip zookeeperport
class KopirpcConfig {
  std::unordered_map<std::string, std::string> configMap;

 public:
  //负责解析加载配置文件
  void LoadConfigFile(const char* config_file);
  //查询配置项信息
  std::string Load(const std::string& key);
};