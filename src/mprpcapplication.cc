#include"mprpcapplication.h"
#include <iostream>
#include<unistd.h>
#include<string>

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
    std::cout << "format : command - i <configfile> " <<  std::endl;
    std::cout << "format : ./provider -i test.conf " << std::endl;
}

void MprpcApplication::Init(int argc, char **argv)
{
    if(argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) !=  -1)
    {
        switch(c)
        {
            case 'i':
                //有配置文件了
                config_file= optarg;
                break;
            case '?': // ? 用于处理未知的选项。
                std::cout << "invalid args!" << std::endl;
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            case ':': // : 用于处理缺少参数的选项。
                std::cout << "need <config file>" << std::endl;
                ShowArgsHelp();
                exit(EXIT_FAILURE);
            default:
                break;
        }
    }

    //开始加载配置文件了 rpcserver_ip= rpcserver_port= zookeeper_ip= zookeeper_port=
    m_config.LoadConfigFile(config_file.c_str());

    // std::cout << "rpcserverip : " << m_config.Load("rpcserverip") << std::endl;
    // std::cout << "rpcserverport : " << m_config.Load("rpcserverport") << std::endl;
    // std::cout << "zookeeperip : " << m_config.Load("zookeeperip") << std::endl;
    // std::cout << "zookeeperport : " << m_config.Load("zookeeperport") << std::endl;
}

MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}
