#pragma once
#include "mprpcconfig.h"
#include "mprpcchannel.h"
#include"mprpccontroller.h"


// mprpc框架的基础类 - 单例, 负责框架的一些初始化操作
class MprpcApplication
{
public:
    static void Init(int argc, char **argv);
    //静态成员函数不需要通过类的实例调用，可以使用类名直接调用。普通成员函数需要通过类的实例才能调用。
    //静态成员函数不会操作任何实例变量，它只能访问静态变量、静态函数和传递的参数。由于没有实例变量的操作，静态成员函数更加简洁和高效。
    //静态成员函数可以直接由类名调用，不需要创建类的实例，使用类名即可调用它，更加方便快捷。
    // 静态成员函数是全局可见的，并可以用作命名空间中的全局函数。
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();

private:
    static MprpcConfig m_config;

    MprpcApplication(){}
    //使用单例模式, 删除所有拷贝构造函数
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&) = delete;


};