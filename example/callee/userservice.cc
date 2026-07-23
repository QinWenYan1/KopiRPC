#include <iostream>
#include <string>


/* 提供一个本地的简单服务*/
class UserService { //使用rpc服务发布端（rpc服务提供者）
    void login(std::string name, std::string pwd){
        std::cout << "Doing the local service: login..."<< std::endl; 
        std::cout << "Name: " << name << " pwd: " << pwd << std::endl; 
    }
    
}; 

int main(){

}
