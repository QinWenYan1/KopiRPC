#include "kopirpcconfig.h"
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

// 解析思路: 单指针两遍扫描,key 与 value 以 '=' 分界
//   key 段: 字母数字收入,空格跳过,遇 '=' 进入 value 段,其余字符报错退出
//   value 段: 数字与 '.' 收入,空格跳过,其余字符报错退出
// configMap.insert 不覆盖已有 key —— 天然实现"重复 key 以第一次为准"
void KopirpcConfig::ReadLineIntoConfigMap(const std::string& line) {
  std::string key, value;
  int index = 0, tail = line.size();

  // 第一遍: 收集 key,停在 '=' 之后
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

  // 第二遍: 收集 value,直到行尾
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

// 找到第一个 '#' 就原地截断: erase 抹掉 [idx, 行尾) 区间
void KopirpcConfig::TrimSharp(std::string& str) {
  if (str.find('#') != std::string::npos) {
    auto idx = str.find('#');
    str.erase(idx, str.size() - idx);
  }
}

// find_first_not_of(' ') 找不到非空格字符,说明整行是空白
bool KopirpcConfig::BlankLine(const std::string& str) {
  return (str.find_first_not_of(' ') == std::string::npos);
}

// 每行的处理流水线: 先截注释,再按是否含 '=' 分流
//   含 '='            -> ReadLineIntoConfigMap 解析入库
//   不含 '=' 且非空白 -> 有内容却没有等号,格式非法,报错退出
//   空白行            -> 直接跳过
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
    //先截注释: '#' 后可能含 '=',不截断会把注释行误判为配置行
    TrimSharp(line);
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

// 未命中时按头文件契约返回空字符串
std::string KopirpcConfig::Load(const std::string& key) const {
  auto it = configMap.find(key);
  if (it == configMap.end()) return "";
  return it->second;
}
