#include "kopirpcapplication.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <fstream>
#include <string>

// 被测行为(见 kopirpcapplication.cc): Init 三段式
//   argc<2 -> 打印用法后 exit(EXIT_FAILURE)
//   getopt 解析 -i <configfile>(非法选项/缺参数/没给选项 -> 退出)
//   加载配置文件并 LOG_INFO 回显(会带动 logger 写当天日志文件)
// 注意: argv 必须用可写 char[],getopt 会原地重排参数,不能用字符串字面量

// ① 单例: 两次获取必须是同一个对象
TEST(KopirpcApplicationTest, SingletonSameInstance) {
  EXPECT_EQ(&KopirpcApplication::GetInstance(),
            &KopirpcApplication::GetInstance());
}

// ② 正例: -i 指定合法配置文件 -> 四个约定 key 全部命中
//   全进程只调这一次成功 Init(getopt 的全局 optind 不复位,第二次调用行为未定义)
TEST(KopirpcApplicationTest, InitWithValidArgsLoadsConfig) {
  const char* path = "/tmp/kopirpc_app_test.conf";
  {
    std::ofstream out(path);
    out << "rpcserverip=127.0.0.1\n"
           "rpcserverport=8000\n"
           "zookeeperip=127.0.0.1\n"
           "zookeeperport=2181\n";
  }

  char arg0[] = "runTests";
  char arg1[] = "-i";
  char arg2[] = "/tmp/kopirpc_app_test.conf";
  char* argv[] = {arg0, arg1, arg2};
  KopirpcApplication::Init(3, argv);

  const KopirpcConfig& cfg = KopirpcApplication::GetConfigFile();
  EXPECT_EQ(cfg.Load("rpcserverip"), "127.0.0.1");
  EXPECT_EQ(cfg.Load("rpcserverport"), "8000");
  EXPECT_EQ(cfg.Load("zookeeperip"), "127.0.0.1");
  EXPECT_EQ(cfg.Load("zookeeperport"), "2181");
}

// ③ 无参数: argc=1 -> 打印用法后以 EXIT_FAILURE 退出
TEST(KopirpcApplicationTest, InitWithoutArgsExits) {
  char arg0[] = "runTests";
  char* argv[] = {arg0};
  EXPECT_EXIT(KopirpcApplication::Init(1, argv),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

// ④ 非法选项: getopt 返回 '?' -> 打印用法后退出
TEST(KopirpcApplicationTest, InitWithUnknownOptionExits) {
  char arg0[] = "runTests";
  char arg1[] = "-x";
  char* argv[] = {arg0, arg1};
  EXPECT_EXIT(KopirpcApplication::Init(2, argv),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}

// ⑤ 配置文件没带 -i: getopt 找不到选项返回 -1,走 else 分支退出
//   (源码注释专门提过的坑: "./provider test.conf" 也必须有错误提示)
TEST(KopirpcApplicationTest, InitWithBareConfigArgExits) {
  char arg0[] = "runTests";
  char arg1[] = "test.conf";
  char* argv[] = {arg0, arg1};
  EXPECT_EXIT(KopirpcApplication::Init(2, argv),
              ::testing::ExitedWithCode(EXIT_FAILURE), "");
}
