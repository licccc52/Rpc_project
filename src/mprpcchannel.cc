#include"mprpcchannel.h"
#include<string>
#include"rpcheader.pb.h"
#include<sys/socket.h>
#include<sys/types.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unistd.h>
#include<error.h>
#include"mprpcapplication.h"
#include"mprpccontroller.h"
#include"zookeeperutil.h"
/*
header_size + rpc_header_str(service_name method_name args_size) + args
*/
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                          google::protobuf::Message* response, google::protobuf::Closure* done)
{
    const google::protobuf::ServiceDescriptor* sd = method->service(); //为了得到相应的service描述符
    std::string service_name = sd->name(); //service_name, 类名字
    std::string method_name = method->name(); //method_name

    //获取参数的序列化字符串长度 args_size
    std::string args_str;
    uint32_t args_size = 0;
    if(request->SerializeToString(&args_str)) //把request序列化, 结果储存在args_str中
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }


    //定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str)) //把rpc_header序列化, 结果储存在rpc_header_str中
    {
        header_size = rpc_header_str.size(); 
        // mprpc::RpcHeader rpcHeader2;
        // if(!rpcHeader2.ParseFromString(rpc_header_str)){
        //     std::cout << " rpcHeader2.ParseFromString失败 " << std::endl;
        // };

    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    //组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char*)&header_size, 4)); // header_size, 4代表要插入的字节数
    LOG_INFO("MprpcChannel::CallMethod : send_rpc_str : %s", send_rpc_str.c_str()); 
    send_rpc_str += rpc_header_str; //rpcheader, 序列化之后的string
    send_rpc_str += args_str; //args,   序列化之后的string

    //打印调试信息
    std::cout << "=====================================" << std::endl;
    std::cout << " header_size :" << header_size << std::endl;
    std::cout << " rpc_header_str :" << rpc_header_str << std::endl;
    std::cout << " service_name :" << service_name << std::endl;
    std::cout << " method_name :" << method_name << std::endl;
    std::cout << " args_str :" << args_str << std::endl;
    std::cout << "=====================================" << std::endl;

    // uint32_t message_size = send_rpc_str.size();
    // uint32_t network_size = htonl(message_size);
    // std::string size_str(reinterpret_cast<const char*>(&network_size), sizeof(network_size));
    // send_rpc_str.append(size_str);

    // LOG_INFO("MprpcChannel::CallMethod : send_rpc_str.size() : %d", message_size);
    LOG_INFO("MprpcChannel::CallMethod : send_rpc_str.size() : %ld", send_rpc_str.size()); 
    LOG_INFO("MprpcChannel::CallMethod : --------------------------------------------");

    //此处是客户端 使用tcp编程, 完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        char errtxt[512] = {0};
        sprintf(errtxt,"create socket error! errno :  %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 读取配置文件rpcserver的信息
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    // rpc调用方想调用service_name的method_name服务，需要查询zk上该服务所在的host信息
    ZkClient zkCli; //zkClient
    zkCli.Start();
    // 组织节点路径 /UserServiceRpc/Login
    std::string method_path = "/" + service_name + "/" + method_name;
    // 127.0.0.1:8000
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str()); 

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    //连接rpc服务节点
    if(-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt,"connect error! errno :  %d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    
    //发送rpc请求 
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt,"send error! errno :  %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    //接收rpc请求的响应值
    char recv_buf[1024] = {0};
    int recv_size = 0;
    if(-1 == (recv_size = recv(clientfd, recv_buf, 1024, 0)))
    {
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "receive error! errno :  %d", errno);
        controller->SetFailed(errtxt);
        return;
    }
    printf("mprpcchannel: running, recv_size = %d\n", recv_size);
    //反序列化rpc调用的响应数据
    std::string response_str(recv_buf,recv_size);
    if (!response->ParseFromArray(recv_buf, recv_size)) {
        close(clientfd);
        std::string errtxt = "parse error! response_str: " + response_str;
        controller->SetFailed(errtxt);
        return;
    }


    close(clientfd);

}