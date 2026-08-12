#aux懒加载，不会自动识别新添加的文件
#改为set方式，手动添加，防止识别到垃圾文件避免意外
set(SRC_LIST  
    kopirpcapplication.cc kopirpcchannel.cc 
    kopirpcconfig.cc rpcheader.pb.cc 
    rpcprovider.cc
    kopirpccontroller.cc
    logger.cc
    zookeeperutil.cc
    )
#静态库
add_library(KopiRPC ${SRC_LIST}) 
#注意链接顺序不能乱了
target_link_libraries(KopiRPC muduo_net muduo_base pthread) 