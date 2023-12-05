#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>
#include "thread.h"

namespace sylar
{

    class Fiber : public std::enable_shared_from_this<Fiber>
    {
    public:
        typedef std::shared_ptr<Fiber> ptr;

        enum State
        {
            INIT,   // 初始状态
            HOLD,   // 挂起状态(暂停状态)
            EXEC,   // 运行状态(执行中状态)
            TERM,   // 结束状态
            READY,  // 就绪状态(可执行状态)
            EXCEPT, // 异常结束状态
        };

    private:
        /**
         * 无参构造函数，每个线程的第一个协程使用该函数构造(主协程)
         */
        Fiber();

    public:
        /**
         * @brief 构造函数创建一个协程
         *
         * @param cb 协程执行的函数
         * @param stacksize 协程栈大小
         */
        Fiber(std::function<void()> cb, size_t stacksize = 0);

        /**
         * @brief Destroy the Fiber object
         *
         */
        ~Fiber();

        /**
         * 重置协程，重新分配一个函数，状态重置
         * 为了内存复用
         */
        void reset(std::function<void()> cb);

        /**
         * 把正在运行的协程放到后台，运行该协程
         */
        void swapIn();

        /**
         * 把当前协程放到后台，将主协程唤醒，运行主协程
         */
        void swapOut();

        u_int64_t getId() const
        {
            return m_id;
        }
        /**
         * @brief G返回协程状态
         *
         * @return State
         */
        State getState() const { return m_state; }

    public:
        /**
         * @brief 设置当前协程为f
         *
         * @param f
         */
        static void SetThis(Fiber *f);
        /**
         * 返回当前执行的协程
         * 如果没有则创建主协程并返回主协程
         */
        static Fiber::ptr GetThis();
        /**
         * 将当前正在执行的协程
         * 切换到后台并且设置为Ready状态
         */
        static void YieldToReady();

        /**
         * 将当前正在执行的协程
         * 切换到后台并且设置为Hold状态
         */
        static void YieldToHold();

        /**
         * 总协程数
         */
        static uint64_t TotalFibers();

        /**
         * 协程所要执行的主函数
         */
        static void MainFunc();

        /**
         * 获取协程ID
         */
        static uint64_t GetFiberId();

    private:
        uint64_t m_id = 0;        // 协程ID
        uint32_t m_stacksize = 0; // 栈大小
        State m_state = INIT;     // 协程状态

        ucontext_t m_cxt; // 保存上下文

        void *m_stack = nullptr;    // 协程栈
        std::function<void()> m_cb; // 协程执行的函数
    };

}

#endif