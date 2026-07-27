#pragma once
#include <string>
#include <unordered_map>

// KopirpcConfig —— 框架配置文件的加载与查询
//
// 配置文件为 key=value 文本格式(见 bin/test.conf),解析规则:
//   1. '#' 起为注释,截断丢弃;空行/纯空格行跳过
//   2. key 仅允许字母与数字;value 仅允许数字与 '.'(适配 IP 与端口)
//   3. 忽略 key/value 两侧的空格;同一 key 重复出现时以第一次为准
//   4. 出现任何非法内容: 打印原因并 exit(EXIT_FAILURE) —— 配置错误零容忍
//
// 框架约定配置项: rpcserverip / rpcserverport / zookeeperip / zookeeperport
// 线程约定: 启动期一次性加载完成,之后各线程只读,无需加锁
class KopirpcConfig {
  // 配置项存储: key -> value
  std::unordered_map<std::string, std::string> configMap;

 public:
  // 打开并解析配置文件,将全部合法条目读入内存
  //   config_file: 配置文件路径(来自命令行 -i)
  //   失败行为: 文件打不开或内容非法时,打印原因后 exit(EXIT_FAILURE)
  void LoadConfigFile(const char* config_file);

  // 按 key 查询配置值
  //   返回: 命中则返回 value;未命中返回空字符串 "",调用方需自行判空
  std::string Load(const std::string& key) const;

 private:
  // ---- 以下为 LoadConfigFile 的私有工具函数 ----

  // 将一行 "key=value" 解析写入 configMap;字符集非法则报错退出
  void ReadLineIntoConfigMap(const std::string&);

  // 就地截断: 删除 str 中第一个 '#' 及其后的全部内容
  void TrimSharp(std::string& str);

  // 判断该行是否为空白行(空串或仅含空格)
  bool BlankLine(const std::string& str);
};
