#pragma once /*防止头文件被包含多次*/
#include "kopirpcconfig.h"

// KopirpcApplication —— 框架基础类(单例)
//
// 职责: 框架的初始化入口 + 全局唯一的配置访问点
//   1. 进程启动时在 main() 里调用一次 Init(): 解析命令行并加载配置文件
//   2. 之后任意组件通过 GetInstance().GetConfigFile() 只读查询配置
// 单例约束: 构造函数私有,拷贝/移动全部禁用,唯一实例由 GetInstance() 持有
class KopirpcApplication {
 public:
  // 框架初始化: 解析命令行参数(必填 -i <configfile>)并加载配置
  //   失败行为: 参数缺失或非法时,打印用法后 exit(EXIT_FAILURE)
  static void Init(int argc, char** argv);

  // 获取全局唯一实例(首次调用时构造)
  static KopirpcApplication& GetInstance();

  // 获取配置对象(只读);查询示例: GetConfigFile().Load("rpcserverip")
  static const KopirpcConfig& GetConfigFile();

 private:
  // 全局唯一的配置存储,Init() 时完成加载
  static KopirpcConfig config;

  // ---- 单例保障: 禁止外部构造、拷贝与移动 ----
  KopirpcApplication() = default;
  KopirpcApplication(const KopirpcApplication&) = delete;
  KopirpcApplication(const KopirpcApplication&&) = delete;

  // 打印命令行用法提示(参数错误时调用)
  static void ShowArgsHelp();
};
