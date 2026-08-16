#单元测试: 纯逻辑组件(config / LockQueue),不碰网络与ZK
#产物runTests进bin/,不进lib/ —— 出包零污染,本子目录常开不注释

#22.04的libgtest-dev自带编好的.a和CMake配置,可直接find_package
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

#config的实现只依赖标准库,直接把它编进测试程序,
#不链libKopiRPC.a —— 避免拖入muduo/zk/protobuf整条依赖链
add_executable(runTests
  test_kopirpcconfig.cc
  test_lockqueue.cc
  ${PROJECT_SOURCE_DIR}/src/kopirpcconfig.cc)

#头文件搜索路径继承顶层的include_directories(src/include)
target_link_libraries(runTests GTest::gtest GTest::gtest_main Threads::Threads)
