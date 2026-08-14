# !/bin/bash
# 需要给别人提供头文件作为API
# 其次将 静态库 + 头文件 都放在lib这个文件夹之下

set -e

rm -rf `pwd`/build/* #先清理
cd `pwd`/build && 
    cmake .. &&
    make 
cd ..

cp -r `pwd`/src/include `pwd`/lib #拷贝过去