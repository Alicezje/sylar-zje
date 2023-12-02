#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
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

#include "mutex.h"

// 基于pthread实现

namespace sylar
{

    class Thread : Noncopyable
    {
    public:
        typedef std::shared_ptr<Thread> ptr;

        /**
         * 创建一个线程
         * 传入(线程指定函数,线程名称)
         */
        Thread(std::function<void()> cb, const std::string &name);

        /**
         * 析构进程
         */
        ~Thread();

        /**
         * 等待线程执行完成
         */
        void join();

        /**
         * 获取当前线程指针
         */
        static Thread *GetThis();

        /**
         * 获得当前线程名称
         */
        static const std::string GetName();

        /**
         * 设置当前线程名称
         */
        static void SetName(const std::string &name);

        /**
         * 获取进行ID
         */
        pid_t getId() const
        {
            return m_id;
        }

        /**
         * 获取进程名称
         */
        const std::string getName() const
        {
            return m_name;
        }

    private:
        /**
         * 运行线程
         */
        static void *run(void *arg);

    private:
        pid_t m_id = -1;            // 线程id
        pthread_t m_thread = 0;     // 线程结构(通过pthread_create创建得到的)
        std::function<void()> m_cb; // 线程所执行的函数
        std::string m_name;         // 线程名称
        Semaphore m_semaphore;      // 信号量，用于互斥
    };
}

#endif