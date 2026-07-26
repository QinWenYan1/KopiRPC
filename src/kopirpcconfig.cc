#include "kopirpcconfig.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

//私人工具函数: 将一列配置信息读入到map当中
void KopirpcConfig::ReadLineIntoConfigMap(const std::string& line) {
  std::string key, value;
  int index = 0, tail = line.size();

  // key处理
  while (index != tail) {
    if (line[index] == '=') {
      ++index;
      break;
    }
    if (isalnum(line[index])) {
      key.push_back(line[index]);
    } else if (line[index] != ' ') {
      std::cout << ": illegal key in entries, please try it again" << std::endl;
      exit(EXIT_FAILURE);
    }
    ++index;
  }

  // value处理
  while (index != tail) {
    if (line[index] == '.' || isdigit(line[index])) {
      value.push_back(line[index]);
    } else if (line[index] != ' ') {
      std::cout << ": illegal value in entries, please try it again"
                << std::endl;
      exit(EXIT_FAILURE);
    }
    ++index;
  }
  configMap.insert({key, value});
}

//私人工具函数: 将包括#字符及其以后全部删除
void KopirpcConfig::TrimSharp(std::string& str) {
  if (str.find('#') != std::string::npos) {
    auto idx = str.find('#');
    str.erase(idx, str.size() - idx);
  }
}

//私人工具函数: 检查此行是不是只有空格或者empty
bool KopirpcConfig::BlankLine(const std::string& str) {
  return (str.find_first_not_of(' ') == std::string::npos);
}

//负责解析加载配置文件
void KopirpcConfig::LoadConfigFile(const char* config_file) {
  /*打开文件输入流*/
  std::ifstream in(config_file, std::fstream::in);

  if (!in.is_open()) {
    std::cout << config_file << ": is not existed!" << std::endl;
    exit(EXIT_FAILURE);
  }

  /*成功打开文件，开始一行一行阅读*/
  std::string line;
  while (getline(in, line)) {
    //删除#字符
    TrimSharp(line);
    //情况：不是注释行，开始读入
    if (line.find("=") != std::string::npos)
      ReadLineIntoConfigMap(line);
    else if (!BlankLine(line)) {
      std::cout << config_file << ": no equation, please try it again"
                << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  /*关闭文件输入流*/
  if (in.is_open()) in.close();
}

//查询配置项信息
std::string KopirpcConfig::Load(const std::string& key) {
  auto it = configMap.find(key);
  //不存在key
  if (it == configMap.end()) return "";
  return it->second;
}