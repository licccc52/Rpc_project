#pragma once
#include "google/protobuf/service.h"
#include<string>

#include<functional>
#include<google/protobuf/descriptor.h>
#include<../tcpproject/EchoServer.h>
#include<../tcpproject/EventLoop.h>
#include<../tcpproject/InetAddress.h>
#include<../tcpproject/Connection.h>
#include<unordered_map>
#include"logger.h"

//框架提供的专门发布rpc服务的网络对象类
//转用于发布的rpc网络框架服务类
//负责数据的序列化反序列化, IO收发

class RpcProvider{
public:
    //这里是框架提供给外部使用的, 可以发布rpc方法的函数接口
    void NotifyService(google::protobuf::Service *service);

    //启动rpc服务节点, 开始提供rpc远程网络调用服务
    void Run();

private:
    
    //组合EventLoop, 在TcpServer中已经包含EventLoop了
    // EventLoop m_eventLoop;

    //service服务类型信息
    struct ServiceInfo
    {
        google::protobuf::Service *m_service;//保存服务对象
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> m_methodMap;  //保存服务方法
    };
    //存储注册成功的服务对象和其服务方法的所有信息
    std::unordered_map<std::string, ServiceInfo> m_serviceMap;

    //新socket连接回调
    // void OnConnection(spConnection conn,std::string& recv_buf);
    //已建立连接用户的读写时间回调
    void OnMessage(spConnection conn,std::string& recv_buf);

    // CLosure的回调操作, 用于序列化rpc的响应和网络发送
    void SendRpcResponse(spConnection conn, google::protobuf::Message*response);
};