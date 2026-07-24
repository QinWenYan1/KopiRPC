#pragma once /*防止头文件被包含多次*/

/*KopiRPC框架的基础类*/
/*设计为单例模式，方便各个组件能十分方便的获取配置信息*/
class KopirpcApplication
{
public: 
    static void Init(int argc, char **argv);
private:
    KopirpcApplication();
    /*拷贝构造函数全部删除*/
    KopirpcApplication(const KopirpcApplication&) = delete;
    KopirpcApplication(const KopirpcApplication&&) = delete;
}; 