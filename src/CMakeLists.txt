aux_source_directory(. SRC_LIST)
#静态库
add_library(KopiRPC ${SRC_LIST}) 
target_link_libraries(KopiRPC muduo_net muduo_base pthread) 