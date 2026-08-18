#压测客户端: 阶段1 benchmark 的压测工具
#产物bench进bin/不进lib/;只在example打开时构建,出包态随example一起被排除
set(SRC_LIST bench.cc ../friend.pb.cc)
add_executable(bench ${SRC_LIST})
target_link_libraries(bench protobuf KopiRPC)
