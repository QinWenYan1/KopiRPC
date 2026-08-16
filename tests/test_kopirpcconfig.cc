#include "kopirpcconfig.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>

// 被测契约(见 kopirpcconfig.h 头注释):
//   '#' 注释截断;空白行跳过;key 仅字母数字;value 仅数字与 '.'
//   key=value 两侧空格忽略;重复 key 取第一次;Load 未命中返回 ""
//   任何非法内容 -> 打印原因后 exit(EXIT_FAILURE),零容忍

// 把 content 写成一个临时配置文件,返回路径(每个用例一个文件,互不干扰)
static std::string WriteTempConf(const char* name, const char* content) {
  std::string path = std::string("/tmp/kopirpc_cfg_") + name + ".conf";
  std::ofstream out(path);
  out << content;
  return path;  // out 析构时关闭落盘
}

// ---------- 正常解析(5 例) ----------

// key/value 两侧的空格被忽略
TEST(KopirpcConfigTest, SpacesAroundKeyValueIgnored) {
  KopirpcConfig cfg;
  cfg.LoadConfigFile(
      WriteTempConf("spaces", "rpcserverip = 127.0.0.1\nrpcserverport= 8000\n")
          .c_str());
  EXPECT_EQ(cfg.Load("rpcserverip"), "127.0.0.1");
  EXPECT_EQ(cfg.Load("rpcserverport"), "8000");
}

// 行尾 '#' 起为注释,截断后不影响解析
TEST(KopirpcConfigTest, TrailingSharpCommentTrimmed) {
  KopirpcConfig cfg;
  cfg.LoadConfigFile(
      WriteTempConf("sharp", "rpcserverport=8000 # 服务端端口\n").c_str());
  EXPECT_EQ(cfg.Load("rpcserverport"), "8000");
}

// 整行注释与空白行(含纯空格行)直接跳过
TEST(KopirpcConfigTest, CommentAndBlankLinesSkipped) {
  KopirpcConfig cfg;
  cfg.LoadConfigFile(
      WriteTempConf("blank", "# 整行注释\n\n   \nrpcserverport=8000\n")
          .c_str());
  EXPECT_EQ(cfg.Load("rpcserverport"), "8000");
}

// 同一 key 重复出现以第一次为准(configMap.insert 不覆盖已有 key)
TEST(KopirpcConfigTest, DuplicateKeyFirstWins) {
  KopirpcConfig cfg;
  cfg.LoadConfigFile(
      WriteTempConf("dup", "rpcserverport=8000\nrpcserverport=9999\n").c_str());
  EXPECT_EQ(cfg.Load("rpcserverport"), "8000");
}

// Load 未命中返回空字符串,调用方自行判空
TEST(KopirpcConfigTest, LoadMissReturnsEmpty) {
  KopirpcConfig cfg;
  cfg.LoadConfigFile(WriteTempConf("miss", "rpcserverport=8000\n").c_str());
  EXPECT_EQ(cfg.Load("zookeeperport"), "");
}

// ---------- 非法输入零容忍: 进程必须以 EXIT_FAILURE 退出(3 例) ----------
// 错误信息打在 std::cout 而非 stderr,匹配串给 "";断言的是退出码本身

// 有内容却没有 '=' 的垃圾行
TEST(KopirpcConfigTest, GarbageLineWithoutEqualExits) {
  KopirpcConfig cfg;
  std::string path = WriteTempConf("garbage", "this is not an entry\n");
  EXPECT_EXIT(cfg.LoadConfigFile(path.c_str()),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

// value 含非法字符(契约只允许数字与 '.')
TEST(KopirpcConfigTest, IllegalValueCharExits) {
  KopirpcConfig cfg;
  std::string path = WriteTempConf("badchar", "rpcserverport=80a0\n");
  EXPECT_EXIT(cfg.LoadConfigFile(path.c_str()),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

// 配置文件不存在
TEST(KopirpcConfigTest, ConfigFileNotFoundExits) {
  KopirpcConfig cfg;
  EXPECT_EXIT(cfg.LoadConfigFile("/tmp/kopirpc_cfg_no_such_file.conf"),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}
