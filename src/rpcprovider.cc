#include"rpcprovider.h"
#include"mprpcapplication.h"
#include"rpcheader.pb.h"
#include"zookeeperutil.h"
/*
service_name => service 描述 => 
                        service 描述 => service* 记录服务对象
                        method_name => method方法对象
*/  

// 这里是框架提供给外部使用的, 可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{

    ServiceInfo service_info;

    //获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();

    //获取服务的名字
    std::string service_name = pserviceDesc->name();

    //获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    // std::cout << "RpcProvider::NotifyService ,service_name: " << service_name <<std::endl;
    LOG_INFO("RpcProvider::NotifyService ,service_name : %s", service_name.c_str());

    for(int i=0; i < methodCnt; ++i)
    {
        //获取了服务对象指定下标的服务方法的描述(抽象描述)
        const google::protobuf::MethodDescriptor* pmethodDesc = pserviceDesc->method(i);
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        // std::cout << "RpcProvider::NotifyService, method_name: " << method_name <<std::endl;
        LOG_INFO("RpcProvider::NotifyService ,method_name : %s", method_name.c_str());    
    }
    service_info.m_service = service;//服务对象, 之后可以用该服务对象调用相应的map中的服务方法
    m_serviceMap.insert({service_name, service_info});
}


//启动rpc服务节点, 开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    InetAddress address(ip, port);

    //创建TcpServer对象
    EchoServer server(ip, port, 3, 3);//只创建了工作
    
    //绑定连接回调 和 消息读写回调方法, 用muduo库 很好地 分离了网络代码和业务代码
    
    //绑定回调函数 处理新用户地连接
    // server.getTcpServer()->setnewconnectioncb(std::bind(&RpcProvider::OnConnection, this))
    
    //绑定回调函数 处理已经连接用户的读写事件
    server.getTcpServer()->setonmessagecb(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    // server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));


    //设置muduo库的线程数量-> muduo会根据数量自动分发IO线程和工作线程, 如果线程数量是1,那么IO线程和工作线程在一个线程中
    // server.setThreadNum(4);
    
    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // 默认会话事件 session timeout   30s,     zkclient 网络I/O线程  1/3 * timeout 时间发送ping消息
    ZkClient zkCli;
    zkCli.Start(); //会自动发送心跳消息
    // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : m_serviceMap) 
    {
        // 取得/service_name   /UserServiceRpc ,创建节点
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0);
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name   /UserServiceRpc/Login(节点) 然后给这个节点存储当前这个rpc服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);//创建一个临时性节点, zkclientid_t析构之后删除节点
        }
    }


    // std::cout << "RpcProvider::Run(), RpcProvider start service at ip: " << ip << " port: " << port << std::endl;  
    LOG_INFO("RpcProvider::Run(), RpcProvider start service at ip: %s, port : %d", ip.c_str(), port);    

    //启动网络服务
    server.Start();
}


// //新socket连接回调
// //rpc请求是一个短链接的请求, 类似http,请求一次结束之后, 服务端返回rpc方法的响应之后会主动关闭连接
// void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
// {
//     if(!conn->connected())
//     {
//         //rpc client的连接断开了
//         // std::cout << "RpcProvider::OnConnection, New Connection Comming;  getTcpInfoString() : " << conn->getTcpInfoString() << std::endl;
//         conn->shutdown();
//     }
// }
/*
在框架内部, RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
携带过来的数据的形式 : service_name method_name args =>> 定义proto的message类型, 进行数据头的序列化和反序列化

header_size(4个字节) + headser_str + args_str
4个字节中包含了除了参数以外的所有信息 : service_name method_name
*/

//已建立连接用户的读写时间回调, 如果远程有一个rpc服务的调用请求, 那么OnMessage方法就会相应
void RpcProvider::OnMessage(spConnection conn,std::string& recv_buf)
{
    LOG_INFO("RpcProvider::OnmMessage : std::string& recv_buf : %s", recv_buf.c_str());    
    //网络上接收的远程rpc调用请求的字符流  Login args
    // std::string recv_buf = buffer->retrieveAllAsString(); //缓冲区中提取所有的数据，并以字符串形式返回

    //从字节流中读取前四个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);//str1.copy(str2,len,pos);
    LOG_INFO("RpcProvider::OnmMessage : header_size : %d", header_size);  
    //根据header_size读取数据头的原始字符流
    //这其中包含了service_name, method_name, args
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    //之后再反序列化数据, 得到rpc请求的详细信息
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str)) //将字符串 rpc_header_str 中的内容解析为 rpcHeader 对象
    {
        //数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();   
        LOG_INFO("RpcProvider::OnmMessage : rpc_header_str : %s, Deserialization SUCCESS!", rpc_header_str.c_str());
    }
    else
    {
        //数据头反序列化失败
        // std::cout << "header_size" << header_size  << std::endl;
        // std::cout << "RpcProvider::OnmMessage : rpc_header_str: " << rpc_header_str << " parse error!" << std::endl;
        LOG_INFO("RpcProvider::OnmMessage : rpc_header_str : %s, Deserialization failed!", rpc_header_str.c_str());    
        
        return;
    }

    //获取rpc方法参数的字符流数据
    std::string args_str = recv_buf.substr(4 + header_size, args_size);
    LOG_INFO("RpcProvider::OnmMessage : args_str : %s", args_str.c_str());


    //打印调试信息
    std::cout << "=====================================" << std::endl;
    std::cout << " header_size :" << header_size << std::endl;
    std::cout << " rpc_header_str :" << rpc_header_str << std::endl;
    std::cout << " service_name :" << service_name << std::endl;
    std::cout << " method_name :" << method_name << std::endl;
    std::cout << " args_size :" << args_size << std::endl;
    std::cout << "=====================================" << std::endl;

    //获取service对象和method对象
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end())
    {
        // std::cout << service_name << " is not exist!" << std::endl;
        LOG_INFO("%s  is not exist!", service_name.c_str());
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end())
    {
        std::cout << service_name << ": " << method_name << " is not exist!" << std::endl;
        LOG_INFO("%s : %s is not exist!", service_name.c_str(), method_name.c_str());
        return;
    }

    google::protobuf::Service *service = it->second.m_service;   //获取service对象
    const google::protobuf::MethodDescriptor *method = mit->second; //获取method对象

    //生成rpc方法调用的请求request和响应的response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();//产生一个具体的服务对象中某个服务方法的请求类型, 产生一个LoginRequest对象, 返回产生的Request的地址(LoginRequest)
    if(!request->ParseFromString(args_str))
    { 
        std::cout << "request parse error! content: " << args_str <<std::endl;
        LOG_ERR("request parse error!");
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    //在框架上根据远端rpc请求, 调用当前rpc节点上发布的方法

    //给下面的method方法的调用, 绑定一个Closure的回调函数
    google::protobuf::Closure *done =
        google::protobuf::NewCallback<RpcProvider, spConnection, google::protobuf::Message*>(this, &RpcProvider::SendRpcResponse, conn, response);

    //在框架上根据远端rpc请求, 调用当前rpc节点上发布的方法
    //new UserService().Login(controller, request, response, done)
    service->CallMethod(method, nullptr, request, response, done);

}

//Closure的回调操作, 用于序列化rpc响应和网络发送
void RpcProvider::SendRpcResponse(spConnection conn, google::protobuf::Message*response)
{
    if(!response) 
        LOG_INFO("RpcProvider::SendRpcResponse() -> response is nullptr! ");
    static std::string response_str;
    if(response->SerializeToString(&response_str)) //response进行序列化
    {
        std::cout << "response->SerializeToString(&response_str) 结果 " <<response_str << std::endl;
        //序列化成功后, 通过网络把rpc方法执行的结果发送回rpc的调用方
        LOG_INFO("RpcProvider::SendRpcResponse() -> SerializeToString response_str: %s send!", response_str.c_str());
        conn->send(response_str.c_str(),response_str.size());
    }
    else
    {
        LOG_INFO("RpcProvider::SendRpcResponse() -> SerializeToString response_str error! ");
    }
    
}


