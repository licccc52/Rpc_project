#pragma once
#include <queue>
#include <thread>
#include <mutex> 
#include <condition_variable> //条件变量

// 异步写日志的日志队列
template<typename T>
class LockQueue
{
public:
    // 多个worker线程都会写日志queue 
    //由 Logger::Log 调用Push() 
    void Push(const T &data)
    {
        std::lock_guard<std::mutex> lock(m_mutex); //构造lock, 析构自动unlock
        m_queue.push(data);
        m_condvariable.notify_one(); //只唤醒等待队列中的第一个线程 ==>> 第27行 
    }

    // 一个线程读日志queue，写日志文件
    T Pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_queue.empty())
        {   //一直阻塞
            // 日志队列为空，线程进入wait状态
            m_condvariable.wait(lock); //wait会释放锁
        }

        T data = m_queue.front();
        m_queue.pop();
        return data;
    }
private:
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_condvariable; //利用线程间共享的全局变量进行同步
};