#include<iostream>
#include<string>

#include"user.pb.h"
#include"mprpcapplication.h"
#include"rpcprovider.h"

/*
UserService原来是一个本地服务, 提供了两个进程内的本地方法 , Login 和 GetFriendLists
*/

// using namespace fixbug;

class UserService : public fixbug::UserServiceRpc //使用再rpc服务发布端(rpc服务的提供者)
{
public:
    // 处理业务的方法
    bool Login(std::string name, std::string pwd)
    {
        std::cout << "UserService::Login : Doing local service Login" << std::endl;
        std::cout << "UserService::Login : name: " << name << " pwd: " << pwd << std::endl;

        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd)
    {
        std::cout << " doing local service : Register" <<std::endl;
        std::cout << "id : " << id << ", name : " <<name << ", pwd" << pwd << std::endl;
        return true;
    }

    //用户登录 -> 发起一个rpc请求到服务器 -> 先被rpc框架接收 -> rpc框架根据请求和参数匹配函数调用
    // 1. caller ===>> Login(LoginRequest) ==>> muduo  ==>> callee
    // 2. callee ===>> Login(LoginRequest) ==>> 交到下面重写的Login方法上
    //重写基类UserServiceRpc的虚函数, 下面这些方法都是框架直接调用的
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {
        //框架给业务上报了请求参数LoginRequest, 业务获取相应数据做本地业务
        //框架已经自动帮忙把发送过来的东西反序列化了
        std::string name = request->name();
        std::string pwd = request->pwd();

        //做本地业务 
        bool login_result = Login(name, pwd); 

        //把响应写入 写错误码, 错误消息, 返回值
        fixbug::ResultCode *code = response->mutable_result();
        code->set_errmsg("");
        response->set_success(login_result);
        // response->set_success(false);

        //执行回调操作 执行响应对象数据的序列化和网络发送(都是由框架完成)
        //closure是protobuf提供的一个抽象类, run是一个虚函数,后续要定义一个类重写run()函数, 或者用一个lambda
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        bool ret = Register(id, name, pwd);

        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(ret);

        done->Run(); //RpcProvider::SendRpcResponse
    }
};

int main(int argc, char **argv)
{
    
    //调用框架的初始化操作 -> 指定一个config.conf文件 读取 相关的网络服务器的配置中心的 IP 和 PORT
    MprpcApplication::Init(argc, argv);//读取配置文件

    //框架上发布服务
    //把UserService对象发布到rpc节点上
    //provider是一个rpc网络服务对象, 把UserService对象发布到rpc节点上
    RpcProvider provider;
    //callee rpc服务的提供者, 可以把本地服务提供成rpc服务 
    provider.NotifyService(new UserService());

    //启动一个rpc服务发布节点, Run以后, 进程进入阻塞状态, 等待远程的rpc调用请求
    provider.Run();

    return 0;
}