#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <pthread.h>
#include <functional>
#include <memory>
#include <string>
#include <cxxabi.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdint.h>
#include <iomanip>
#include <iostream>

// 基于pthread实现

namespace sylar
{
    class Thread
    {
    public:
        typedef std::shared_ptr<Thread> ptr;

        /**
         * 创建一个线程
         * 传入(线程指定函数,线程名称)
         */
        Thread(std::function<void()> cb, const std::string &name);
        ~Thread();

        /**
         * 阻塞线程
         */
        void join();

        static Thread *GetThis();

        static const std::string GetName();

        static void SetName(const std::string &name);

        pid_t getId() const
        {
            return m_id;
        }

        const std::string getName() const
        {
            return m_name;
        }

    private:
        // 设置为私有，默认为delete，防止拷贝
        Thread(const Thread &) = delete;
        Thread(const Thread &&) = delete;
        Thread operator=(const Thread &) = delete;

        /**
         * 运行线程
        */
        static void *run(void *arg);

    private:
        pid_t m_id = -1;            // 线程id
        pthread_t m_thread = 0;     // 线程ID(通过pthread_create创建得到的)
        std::function<void()> m_cb; // 线程所执行的函数
        std::string m_name;         // 线程名称
    };
}

#endif