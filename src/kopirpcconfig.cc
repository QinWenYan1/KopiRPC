#include "kopirpcconfig.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

//私人工具类: 将一列配置信息读入到map当中
void KopirpcConfig::ReadLineIntoConfigMap(const std::string& line) {
  std::string key, value;
  int index = 0, tail = line.size();
  while (index != tail) {
    if (line[index] == '=') { ++index; break; }
    if (std::isalnum(line[index]) || isdigit(line[index]) ) {
      key.push_back(line[index]);
    }
    ++index;
  }

  while (index != tail) {
    if (line[index] == '.' || isdigit(line[index])) {
      value.push_back(line[index]);
    }
    ++index;
  }
  configMap.insert({key, value});
}

//负责解析加载配置文件
void KopirpcConfig::LoadConfigFile(const char* config_file) {
  /*打开文件输入流*/
  std::ifstream in(config_file, std::fstream::in);

  if (!in.is_open()) {
    std::cout << config_file << " is not existed!" << std::endl;
    exit(EXIT_FAILURE);
  }

  /*成功打开文件，开始一行一行阅读*/
  std::string line;
  while (getline(in, line)) {
    //情况-：是注释行跳过
    if (line.find("#") != std::string::npos) continue;
    //情况二：不是注释行，开始读入
    ReadLineIntoConfigMap(line);
  }
  /*关闭文件输入流*/
  if (in.is_open()) in.close();
}

//查询配置项信息
std::string KopirpcConfig::Load(const std::string& key) {
  auto it = configMap.find(key);
  //不存在key
  if (it == configMap.end()) return "unfound";
  return it->second;
}