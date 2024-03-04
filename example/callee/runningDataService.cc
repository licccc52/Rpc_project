#include <iostream>
#include <string>
#include "runningData.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>
#include"logger.h"
#include <fstream>
#include <sstream>

class data : public runningData::runningDataService
{
public:
    std::vector<double> GetCPULoad() {
        std::vector<double> res;
        double avg_load = -1;
        std::ifstream loadavg_file("/proc/loadavg");
        if (loadavg_file.is_open()) {
            std::string line;
            std::getline(loadavg_file, line);
            std::istringstream iss(line);
            // 第一个值是最近 1 分钟的平均负载
            for(int i=0; i < 3; i++)
            {
                iss >> avg_load;
                res.push_back(avg_load);
            }

            loadavg_file.close();
        }
        return res;
}

    // 重写基类方法, 框架调用
    void GetCPULoad(::google::protobuf::RpcController* controller,
                       const google::protobuf::Empty* request,
                       ::runningData::CPULoad* response,
                       ::google::protobuf::Closure* done)
    {
        std::vector<double> cpuLoad = GetCPULoad();
        for (double load : cpuLoad)
        {
            response->add_avg_load(load);
            printf("cpu avg load = %f\n", load);
        }
        done->Run();
    }


    struct NetworkStats {
        long long RceivePackets;
        long long TransmitBytes;
    };

    NetworkStats getNetworkStats(const std::string& interface) 
    {
        NetworkStats stats = {0, 0};
        std::ifstream file("/proc/net/dev");
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.find(interface) != std::string::npos) {
                    std::istringstream iss(line);
                    // Skipping interface name
                    std::string skip;
                    for (int i = 0; i <= 16; ++i) {
                        iss >> skip;
                        if(i==2)
                        {
                            stats.RceivePackets = std::stoll(skip);
                        }
                        else if(i==10)
                        {
                            stats.TransmitBytes = std::stoll(skip);
                        }
                        // std::cout << skip <<"   ";
                    }
                    // std::cout<< std::endl;
                    // std::cout << "------------------------------" << std::endl;
                    break;
                }
            }
            file.close();
        }

        return stats;
    }

    void GetNetworkStats(::google::protobuf::RpcController* controller,
                       const ::runningData::NetworkInterface* request,
                       ::runningData::NetworkStats* response,
                       ::google::protobuf::Closure* done)
    {
        std::string s = request->interface_name();
        NetworkStats stats = getNetworkStats(s);
        response->set_receive_packets(stats.RceivePackets);
        response->set_transmit_packets(stats.TransmitBytes);
        printf("网卡%s : 接收数据包个数%lld, 发送数据包个数%lld", s.c_str(),stats.RceivePackets, stats.TransmitBytes);
        done->Run();
    }

    double calculateMemoryUsage() {
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        double total_mem = 0.0;
        double free_mem = 0.0;
        
        // 读取文件的每一行
        while (std::getline(meminfo, line)) {
            std::istringstream iss(line);
            std::string key;
            double value;
            
            // 解析每一行的键值对
            if (iss >> key >> value) {
                if (key == "MemTotal:") {
                    total_mem = value;
                } else if (key == "MemFree:" || key == "Buffers:" || key == "Cached:") {
                    // 将“MemFree”，“Buffers”和“Cached”三个字段的值相加以获得空闲内存
                    free_mem += value;
                }
            }
        }
        
        // 计算内存使用率
        double mem_usage = 100.0 * (1.0 - (free_mem / total_mem));
        return mem_usage;
    }

    void CalculateMemoryUsage(::google::protobuf::RpcController* controller,
                       const google::protobuf::Empty* request,
                       ::runningData::MemoryUsage* response,
                       ::google::protobuf::Closure* done)
    {
        double mem = calculateMemoryUsage();
        
        response->set_memory_usage(mem);
        printf("current memory usage = %f\n.", mem);
        done->Run();
    }

};

int main(int argc, char **argv)
{
    // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new data());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.Run();

    return 0;
}