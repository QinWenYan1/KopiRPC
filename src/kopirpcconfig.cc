#include "kopirpcconfig.h"

#include <cstdlib>
#include <iostream>

//负责解析加载配置文件
void LoadConfigFile(const char* config_file) {
  FILE* pf = fopen(config_file, "r");
  if (pf == nullptr) {
    std::cout << config_file << " is not existed!" << std::endl;
    exit(EXIT_FAILURE);
  }
}

//查询配置项信息
std::string Load(const std::string& key) {}