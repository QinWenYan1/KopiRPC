#项目的cmake版本和项目名称
cmake_minimum_required(VERSION 3.16)
project(KopiRPC)

# 调试选项:
# 例子: 
# break kopirpcconfig.cc:17
# run -i test.conf
# set(CMAKE_BUILD_TYPE "Debug")

#生成compile_commands.json(clangd 需要)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#设置可执行文件的输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#设置项目库文件的输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置编译头文件的搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/src/include)
include_directories(${PROJECT_SOURCE_DIR}/example)

#设置项目库文件的搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

#-----------------------进入子目录进行编译------------------------

#服务框架的编译路径，src未来向对外提供为so库
#src包含了Kopirpc框架所有的相关代码
add_subdirectory(src)
#使用服务框架的业务路径
# example包含了Kopirpc框架使用的示例代码
# 当作为框架的出包的情况来说，我们不需要example 进入编译环境
# add_subdirectory(example)