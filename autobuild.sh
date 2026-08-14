#!/bin/bash
# 需要给别人提供头文件作为API
# 其次将 静态库 + 头文件 都放在lib这个文件夹之下
# 如何使用build之后的产物: 
#   1. /include 就放在 其他项目的 user include 下
#   2. libkopirpc.a 放在 user lib 下

set -e

rm -rf $(pwd)/build/* #先清理
mkdir -p $(pwd)/build #防干净环境没有build(已存在则不动)
cd $(pwd)/build && 
    cmake .. &&
    make -j $(nproc)
cd ..

rm -rf $(pwd)/lib/include #防已删头文件在副本里残留
cp -r $(pwd)/src/include $(pwd)/lib #拷贝过去