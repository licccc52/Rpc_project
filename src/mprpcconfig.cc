#include"mprpcconfig.h"
#include<iostream>
#include<string>

//负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if(nullptr == pf)
    {
        std::cout << config_file << " is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 1.注释 2.正确的配置项 =  3. 去掉开头的多余的空格
    while(!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        //去掉字符串前面多余的空格
        std::string read_buf(buf);
        Trim(read_buf);

        //判断#的注释, 并且跳过人为多加的空格
        if(read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        //解析配置项
        int idx = read_buf.find('=');
        if(idx == -1)
        {
            //配置项不合法
            continue;
        }

        std::string key;
        std::string value;
        key = read_buf.substr(0, idx);
        Trim(key);
        int endidx = read_buf.find('\n', idx);
        value = read_buf.substr(idx+1, endidx-idx-1); //substr函数参数(起始下标, 长度)
        Trim(value);
        m_configMap.insert({key, value});
    }

}

//查询配置项信息
std::string MprpcConfig::Load(std::string key)
{
    // return m_configMap[key];
    //此处不能使用中括号, 应为此处如果key不存在, 会在map中增加新的键值对
    auto it = m_configMap.find(key);
    if(it == m_configMap.end())
    {
        return "";
    }
    return it->second;

}

//去掉字符串前后的空格
void MprpcConfig::Trim(std::string &src_buf)
{
    int idx = src_buf.find_first_not_of(' '); //从前往后找第一个空格
    if(idx != -1)
    {
        //说明字符串前面有空格
        //idx为第一个非空格字符的下标
        src_buf = src_buf.substr(idx, src_buf.size()-idx);//截断
    }

    //去掉字符串后面多余的空格
    //从后往前找第一个空格
    idx = src_buf.find_last_not_of(' ');
    if(idx != -1)
    {
        //说明字符串后面有空格
        //idx为第一个非空格字符的下标
        src_buf = src_buf.substr(0, idx+1);//截断
    }
}
