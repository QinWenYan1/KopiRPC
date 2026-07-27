set(SRC_LIST userservice.cc ../user.pb.cc)
add_executable(provider ${SRC_LIST})
target_link_libraries(provider protobuf KopiRPC)
target_link_libraries(provider muduo_net muduo_base pthread)