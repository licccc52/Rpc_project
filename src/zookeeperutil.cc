#include "zookeeperutil.h"
#include "mprpcapplication.h"//连接zkserver需要知道zk的ip地址和端口号
#include <semaphore.h>
#include <iostream>

// 全局的watcher观察器   zkserver给zkclient的通知
void global_watcher(zhandle_t *zh, int type,
                   int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT)  // 回调的消息类型是和会话相关的消息类型
	{
		if (state == ZOO_CONNECTED_STATE)  // zkclient和zkserver连接成功
		{
			sem_t *sem = (sem_t*)zoo_get_context(zh);//从指定的句柄上获取信号量 
            sem_post(sem); //信号量资源 + 1 ==>> sem_wait(&sem); zookeeper_init success
		}
	}
}

ZkClient::ZkClient() : m_zhandle(nullptr)//句柄初始化为空
{
}

ZkClient::~ZkClient()
{
    if (m_zhandle != nullptr)//不为空, 证明句柄已经连接zkserver
    {
        zookeeper_close(m_zhandle); // 关闭句柄，释放资源
    }
}

// 连接zkserver
void ZkClient::Start()
{
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;//拼接字符串, 以固定的模式传入zookeeper_init()函数中
    
	/*
	zookeeper_mt：多线程版本
	zookeeper的API客户端程序提供了三个线程
	1. API调用线程 
	2. 网络I/O线程  pthread_create  poll
	3. watcher回调线程 pthread_create ; 负责zkserver给zkclient通知消息
	// 当客户端接收到zkserver的响应的时候, watcher回调线程也是一个独立的线程
    句柄创建成功返回之后会 调用 回调函数
    */
    // zookeeper_init(IP:port, 一个回调函数, 超时事件, nullptr, nullptr, 0);
    //本地句柄创建成功
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 30000, nullptr, nullptr, 0); //直接创建一个线程进行网络I/O操作, 与API调用线程分开
    //Session establishment is asynchronous, meaning that the * session should not be considered established until (and unless) an * event of state ZOO_CONNECTED_STATE is received.
    if (nullptr == m_zhandle) 
    {
        std::cout << "zookeeper_init error!" << std::endl; //此处线程都没有发起
        exit(EXIT_FAILURE);
    }
    //zookeeper_init() 句柄创建成功, 只代表初始化成功, 网络连接到zkserver还没有收到响应
    //创建一个信号量
    sem_t sem;
    sem_init(&sem, 0, 0); //初始化信号量
    zoo_set_context(m_zhandle, &sem); //设置上下文, 等于给global_watcher() 回调函数传入参数

    //初始化创建的信号量为0, 执行到此处一定为阻塞
    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;//等zookeeper服务器响应返回的时候才会创建成功
}

void ZkClient::Create(const char *path, const char *data, int datalen, int state)
{
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
	// 先判断path表示的znode节点是否存在，如果存在，就不再重复创建了
	flag = zoo_exists(m_zhandle, path, 0, nullptr);
	if (ZNONODE == flag) // 表示path的znode节点不存在
	{
		// 创建指定path的znode节点了
		flag = zoo_create(m_zhandle, path, data, datalen,
			&ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
		if (flag == ZOK)
		{
			std::cout << "znode create success... path:" << path << std::endl;
		}
		else
		{
			std::cout << "flag:" << flag << std::endl;
			std::cout << "znode create error... path:" << path << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// 根据指定的path，获取znode节点的值
std::string ZkClient::GetData(const char *path)
{
    char buffer[64];
	int bufferlen = sizeof(buffer);
	int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
	if (flag != ZOK)
	{
		std::cout << "get znode error... path:" << path << std::endl;
		return "";
	}
	else
	{
		return buffer;
	}
}