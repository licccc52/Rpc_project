#include <iostream>
#include "mprpcapplication.h"
#include "runningData.pb.h"
#include "mprpcchannel.h"
#include "logger.h"
#include <fstream>
#include <sstream>
#include <ctime>
#include <signal.h>
#include <unistd.h>

void Stop(int sig){ //信号2和15的处理函数, 功能是停止服务程序
    printf("sig = %d\n", sig);
    //调用EchoServer::Stop()停止服务
    exit(0);
}

void writeStringToFile(const std::string& str, const std::string& filename) {
    // 获取当前时间
    std::time_t currentTime = std::time(nullptr);
    std::string timeString = std::asctime(std::localtime(&currentTime));
    timeString.pop_back(); // 移除末尾的换行符

    // 打开文件
    std::ofstream outputFile(filename, std::ios::app);

    if (outputFile.is_open()) {
        // 写入当前时间和字符串
        outputFile << timeString << " " << str << std::endl;
        
        // 关闭文件
        outputFile.close();
        std::cout << "数据已写入到文件 " << filename << " 中" << std::endl;
    } else {
        std::cerr << "无法打开文件! " << filename << std::endl;
    }
}

int main(int argc, char**argv)
{
    //整个程序启动以后, 想使用mprpc框架享受rpc服务调用, 一定需要先调用框架的初始化函数(只初始化一次)
    MprpcApplication::Init(argc, argv); //读取conf文件

    signal(SIGTERM, Stop); //信号15, 系统kill或killall命令默认发送的信号
    signal(SIGINT, Stop); //信号2, 按Ctrl+c 发送的信号
    
    // while(true){
        //调用远程发布的rpc方法Login
        const google::protobuf::Empty request;
        runningData::runningDataService_Stub stub(new MprpcChannel());

        //rpc方法的响应 
        runningData::CPULoad response;

        //发起rpc方法的调用 同步rpc调用过程 MprpcChannel::callmethod
        MprpcController controller;
        stub.GetCPULoad(&controller, &request, &response, nullptr); //RpcChannel->RpcChannel : MprpcChannel::CallMethod 集中起来做所有rpc方法调用的参数序列化的网络发送

        //一次CPU rpc调用完成, 读调用的结果
        if(controller.Failed())
        {
            std::cout << controller.ErrorText() << std::endl;
        }
        else
        {
            // int size = response.avg_load_size();
            // std::cout <<"avg_load_size = " <<size<< std::endl;
            std::ostringstream oss;
            oss << "CPU 1 分钟内平均负载 : " << response.avg_load(0) << std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
            oss.str("");
            oss << "CPU 5 分钟内平均负载 : " << response.avg_load(1) << std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
            oss.str("");
            oss << "CPU 15 分钟内平均负载 : " << response.avg_load(2) << std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
            
        }

        runningData::NetworkInterface request2;
        runningData::NetworkStats response2;
        request2.set_interface_name("lo");
        stub.GetNetworkStats(&controller, &request2, &response2, nullptr);
        //一次网卡 rpc调用完成, 读调用的结果
        if(controller.Failed())
        {
            std::cout << controller.ErrorText() << std::endl;
        }
        else
        {
            std::ostringstream oss;
            oss << "网卡" << request2.interface_name() << "接收数据包个数 : " << response2.receive_packets() << "  ," << "发送的数据报个数 : " << response2.transmit_packets() << std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
        }

        request2.set_interface_name("ens33");
        stub.GetNetworkStats(&controller, &request2, &response2, nullptr);
        //一次网卡 rpc调用完成, 读调用的结果
        if(controller.Failed())
        {
            std::cout << controller.ErrorText() << std::endl;
        }
        else
        {
            std::ostringstream oss;
            oss << "网卡 " << request2.interface_name() << " 接收数据包个数 : " << response2.receive_packets() << "  ," << "发送的数据报个数 : " << response2.transmit_packets() << std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
        }

        // const google::protobuf::Empty request;
        //rpc方法的响应
        runningData::MemoryUsage response3;

        stub.CalculateMemoryUsage(&controller, &request, &response3, nullptr);
        if(controller.Failed())
        {
            std::cout << controller.ErrorText() << std::endl;
        }
        else
        {
            std::ostringstream oss;
            oss << "内存利用率 : " << response3.memory_usage() << "%  "<<std::endl;
            writeStringToFile(oss.str(), "ServerRunningData.txt");
        }
        writeStringToFile("---------------------------------------------------------------||", "ServerRunningData.txt");
        // exit(0);
        
    // }
    return 0;
}