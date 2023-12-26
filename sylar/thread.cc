#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar
{
    // thread_local 线程开始时被生成，线程结束时销毁
    // 线程指针
    static thread_local Thread *t_thread = nullptr;
    // 线程名称
    static thread_local std::string t_thread_name = "UNKNOW";

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    Thread *Thread::GetThis()
    {
        return t_thread;
    }

    const std::string Thread::GetName()
    {
        return t_thread_name;
    }

    void Thread::SetName(const std::string &name)
    {
        if (name.empty())
        {
            return;
        }
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    /**
     * 初始化一个线程
     * - 线程运行的函数
     * - 线程名称
     */
    Thread::Thread(std::function<void()> cb, const std::string &name)
        : m_cb(cb), m_name(name)
    {
        if (name.empty())
        {
            m_name = "UNKNOW";
        }
        // 创建线程, 线程执行方法设为run, void* run(void* arg), 参数为this,内部就像参数转为Thread
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail=" << rt << " name= " << name;
            throw std::logic_error("pthread_create error");
        }
        // 申请信号量，如果申请不到，一直等待
        // 从信号量的值-1，如果信号量为0，则需要在这里等待
        m_semaphore.wait();
    }

    Thread::~Thread()
    {
        if (m_thread)
        {
            // 线程分离
            pthread_detach(m_thread);
        }
    }

    void Thread::join()
    {
        if (m_thread)
        {
            // 回收进行资源，阻塞进程
            int rt = pthread_join(m_thread, nullptr);
            if (rt)
            {
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail=" << rt;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    void *Thread::run(void *arg)
    {
        // 参数传入的arg是thread类的this
        Thread *thread = (Thread *)arg; // 转成Thread
        t_thread = thread;              // 给thread_local变量赋值
        thread->m_id = sylar::GetThreadId();
        // 设置当前线程的名称
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> cb;
        // 交换地址
        cb.swap(thread->m_cb);
        // 释放信号量
        thread->m_semaphore.notify();
        // 执行函数
        cb();
        return 0;
    }

}