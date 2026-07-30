#include <iostream>
#include "KopirpcApplication.h"

/*
* example属于是业务代码，包括caller + callee
*/

int main(int argc, char* argv[]){
    //整个程序启动以后，想要使用KopiRpc框架服务，一定要调用框架的初始化函数
    //只需要初始化一次
    KopirpcApplication::Init(argc, argv);

    return 0; 

}