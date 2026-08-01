#set(SRC_LIST calleruserservice.cc ../user.pb.cc)
set(SRC_LIST callerfriendservice.cc ../friend.pb.cc)
add_executable(consumer ${SRC_LIST})
target_link_libraries(consumer protobuf KopiRPC)