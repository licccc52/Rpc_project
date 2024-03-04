#pragma once
#include<unordered_map>
#include<string>
//框架的读取配置文件类
//读取配置文件保存再键值对map中

//rpcserver ip, rpcserverport  ||  zookeeperip zookeeperport
class MprpcConfig
{ //键值对

public:
    //负责解析加载配置文件
    void LoadConfigFile(const char *config_file);

    //查询配置项信息
    std::string Load(std::string key);

private:

    std::unordered_map<std::string, std::string> m_configMap;

    //去掉字符串前后的空格
    void Trim(std::string &src_buf);
};
