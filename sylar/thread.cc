#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar
{
    static thread_local Thread *t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";

    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    Thread *Thread::GetThis()
    {
        return nullptr;
    }

    const std::string Thread::GetName()
    {
        return "";
    }

    void Thread::SetName(const std::string &name)
    {
        if (t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    /**
     * 初始化一个线程
     * 线程运行的函数
     * 线程名称
     */
    Thread::Thread(std::function<void()> cb, const std::string &name)
    {
        if (name.empty())
        {
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "pthread_create thread fail=" << rt << " name= " << name;
            throw std::logic_error("pthread_create error");
        }
    }

    Thread::~Thread()
    {
        if (m_thread)
        {
            pthread_detach(m_thread);
        }
    }

    void Thread::join()
    {
        if (m_thread)
        {
            // 阻塞主线程，去执行该子线程
            int rt = pthread_join(m_thread, nullptr);
            if (rt)
            {
                SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail=" << rt;
                throw std::logic_error("pthread_join error");
            }
            //?
            m_thread = 0;
        }
    }

    void *Thread::run(void *arg)
    {
        Thread *thread = (Thread *)arg;
        t_thread = thread;
        thread->m_id = sylar::GetThreadId();
        pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

        std::function<void()> cb;
        cb.swap(thread->m_cb);

        cb();
        return 0;
    }

}