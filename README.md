# Mprpc + React_Server project2 一个分布式网路通信框架 + Reactor_server服务器
用该项目[https://github.com/licccc52/Reactor_server2]代替原来的muduo库

原项目地址[https://github.com/licccc52/Mprpc_project/tree/main]

### 添加了一些功能
#### 1. 获取服务端的CPU使用率

    /proc/loadavg的前三个数据分别代表CPU过去1,5,15分钟的平均负载

#### 2. 获取服务端的内存使用率
    遍历 /proc/meminfo 中的每一行, 取出每一行的键值对
    把 MemFree, Buffers ,Cached 取出来相加 / total_memory(MemTotal)

#### 3. 获取服务端网卡收发数据情况
    /proc/net/dev文件中一共两行数据,取每一行的第3列与第11列


## 集群 : 每一台服务器独立运行一个工程的所有模块
## 分布式 : 一个工程拆分了很多模块, 每一个模块独立部署运行在一个服务器主机上, 所有服务器协同工作共同提供服务, 每一台服务器称作分布式的一个节点, 根据节点的并发要求, 对一个节点可以再做节点模块集群部署


## 单机版服务器瓶颈:
1. 受限于硬件资源, 聊天服务器所能承受的用户并发量不够
2. 任意模块的修改, 都会导致整个项目代码重新编译, 部署
3. 系统中,有些模块属于CPU密集型(计算量很大)-> 部署在CPU好的机器上, 有些模块属于I/O密集型(经常接收网络IO, 输入输出)-> 需要带宽好, 造成各模块对于硬件资源的需求是不一样的

## 集群服务器, 集群中的每一台服务器都独立运行一套系统
1. 用户并发量提升了
2. 类似后台管理这样的模块, 不需要高并发
3. 项目代码还是需要整体重新编译, 而且需要进行多次部署

## 分布式服务器 : 所有服务器共同构成一个聊天系统, 
1. 大系统的软件模块怎么划分? -> 可能会实现大量重复代码                              \  rpc
2. 各模块之间如何访问?  机器一上的模块怎么调用机器2上的模块的一个业务方法?           /


## protobuf =>> json
1. protobuf二进制存储; xml和json都是文本存储
2. protobuf不需要存储额外的信息

## 项目文件结构
1. ./bin 可执行文件
2. ./build CMake 构建编译项目文件
3. ./examle 使用框架的服务消费者, 框架的使用实例代码
4. ./lib 库文件
5. ./src 源代码
6. ./test 测试

## 提供静态库的方式(因为muduo是静态库), 动态库编译不成功

### static成员变量
静态成员变量必须在类内声明, 在类外初始化


## 程序梳理
服务的提供方会首先通过RpcProvider类注册服务对象和服务方法, 然后RpcProvider会用unordered_map表记录下来, RpcProvider启动之后, 相当于启动了一个Epoll+多线程的服务器; 启动了之后就可以接收远程的连接, 远程有新连接之后, moduo会调用RpcProvider::OnConnection; 如果远程有message, 会调用RpcProvider::Onmessage(), 会从数据头中解析出request参数, response由业务函数填, 然后业务函数执行回调函数发送回去 =>> SendRpcResponse()

### 关键函数
1. Init(),初始化Rpc服务器
void MprpcApplication::Init(int argc, char **argv)

2. //负责解析加载配置文件
void LoadConfigFile(const char *config_file);

3. //查询配置项信息
std::string Load(std::string key);

4. 启动rpc服务节点, 开始提供rpc远程网络调用服务
void RpcProvider::Run()

5. 已建立连接用户的读写时间回调, 如果远程有一个rpc服务的调用请求, 那么OnMessage方法就会相应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buffer,  muduo::Timestamp)
接收传过来的数据流, 反序列化, 解析出service对象和method_name, 执行对应函数, 把结果返回 

6. //启动rpc服务节点, 开始提供rpc远程网络调用服务, 调用muduo网络库, 绑定连接和发送消息的回调函数
void RpcProvider::Run();

7. mprpccontroller类
记录程序运行中的信息,主要是记录各个重要函数运行的状态信息

### 单例模式
在单例模式中， 只有一个对象，类的设计旨在确保一个类只有一个实例，并提供一个全局的访问点来获取该实例。删除拷贝构造函数是为了防止通过拷贝构造函数创建类的多个实例，从而违反了单例模式的初衷

### znode节点存储格式
在ZooKeeper中创建一个ZNode时，可以选择为其分配一些数据。这个数据是以字节数组的形式存储的，因此它可以是任何形式的信息，包括文本、二进制数据等。ZNode的数据通常用于存储配置信息、临时状态、协调信息等。

除了数据之外，每个ZNode还包含一些元数据信息，如：

节点路径（Path）： 这是ZNode在ZooKeeper命名空间中的唯一标识符，类似于文件系统中的路径。例如，/myapp/config。

节点版本号（Version Number）： 每次ZNode的数据发生变化时，版本号都会递增。这个版本号在更新和删除操作中很重要，用于实现乐观锁机制。

ACL（Access Control List）： 它指定了谁有权访问该节点以及对该节点的哪些操作有权限。ACL用于控制对ZNode的读写权限。

时间戳（Timestamp）： 记录了ZNode的创建时间或最后一次修改时间。

ZNode可以以树状结构组织，每个ZNode都可以有多个子节点，这些子节点也是ZNode。这种层次结构允许您在ZooKeeper中构建复杂的数据模型，用于各种分布式系统任务。

### zookeeper客户端常用命令
ls :  罗列当前指定目录节点
get :  查看目录节点内容
create : 创建节点
set : 修改节点的值
delete : 删除节点


### zookeeper的watcher机制
用Service_name + method_name 作为一个路径查找zookeeper中这个节点是否存在判断服务是否存在 : 用路径管理znode节点
事件回调机制.  
Zookeeper提供了分布式数据的发布/订阅功能，可以让客户端订阅某个节点，当节点发生变化 (比如创建、修改、删除、数据获取、子节点获取)时，可以通知所有的订阅者。 另外还可以为客户端连接对象注册监听器，可以监听到连接时的状态。

![RPC通信原理](rpc通信原理.png)
黄色部分: 设计rpc方法参数的打包和解析, 也就是数据的序列化和反序列化, 使用protobuf

绿色部分: 网络部分, 包括寻找rpc服务主机, 发起rpc调用请求和响应rpc调用结果,使用muduo网络库和zookeeper服务配置中心

![RPC项目结构](项目代码交互图-用画图板打开.png)


# 出现bug:
./consumer2 -i test.conf不会正常退出

生成core文件用gdb调试发现缺少文件   
```
../csu/libc-start.c: 没有那个文件或目录.
```
```
get znode error... path:/runningDataService/CalculateMemoryUsage
2024-03-04 15:01:01,520:76305(0x7ffff7763780):ZOO_INFO@zookeeper_close@2526: Closing zookeeper sessionId=0x18e035d82e5015a to [127.0.0.1:2181]

[Thread 0x7ffff6f4d700 (LWP 76716) exited]
[Thread 0x7ffff674c700 (LWP 76715) exited]
115             if(controller.Failed())
(gdb) n
117                 std::cout << controller.ErrorText() << std::endl;
(gdb) n
/runningDataService/CalculateMemoryUsage is not exist!
125             writeStringToFile("---------------------------------------------------------------||", "ServerRunningData.txt");
(gdb) n
数据已写入到文件 ServerRunningData.txt 中
129         return 0;
(gdb) n
112             runningData::MemoryUsage response3;
(gdb) n
81              runningData::NetworkStats response2;
(gdb) n
80              runningData::NetworkInterface request2;
(gdb) n
56              MprpcController controller;
(gdb) n
53              runningData::CPULoad response;
(gdb) n
50              runningData::runningDataService_Stub stub(new MprpcChannel());
(gdb) n
49              const google::protobuf::Empty request;
(gdb) n
130     }
(gdb) n
__libc_start_main (main=0x555555573830 <main(int, char**)>, argc=3, argv=0x7fffffffe148, init=<optimized out>, fini=<optimized out>, rtld_fini=<optimized out>, 
    stack_end=0x7fffffffe138) at ../csu/libc-start.c:342
342     ../csu/libc-start.c: 没有那个文件或目录.
(gdb) n

n
n
q
^C
Thread 1 "consumer2" received signal SIGINT, Interrupt.
futex_wait (private=<optimized out>, expected=12, futex_word=0x555555590604 <Logger::GetInstance()::logger+164>) at ../sysdeps/nptl/futex-internal.h:141
141     ../sysdeps/nptl/futex-internal.h: 没有那个文件或目录.
(gdb) q
A debugging session is active.
```