#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include <list>
#include <vector>
#include <atomic>
#include "mutex.h"
#include "fiber.h"
#include "thread.h"

namespace sylar
{
    /**
     * @brief 协程调度器
     * 封装一个N-M的协程调度器
     * 内部有一个线程池，支持协程在线程池里面切换
     *
     */
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;

        /**
         * @brief 构造函数
         *
         * @param threads 线程数量
         * @param use_caller 是否使用当前调用线程
         * @param name 协程调度器名称
         */
        Scheduler(size_t threads = 1, bool use_caller = true, const std::string &name = "");

        /**
         * @brief 析构函数
         *
         */
        virtual ~Scheduler();

        const std::string &getName() const
        {
            return m_name;
        }

    public:
        /**
         * @brief 返回当前协程调度器
         *
         * @return Scheduler*
         */
        static Scheduler *GetThis();

        /**
         * @brief 返回当前协程调度器的调度协程
         *
         * @return Fiber*
         */
        static Fiber *GetMainFiber();

        /**
         * @brief 启动协程调度器
         *
         */
        void start();

        /**
         * @brief 停止协程调度器
         *
         */
        void stop();

        /**
         * @brief 调度协程
         *
         * @tparam FiberOrCb
         * @param fc 协程或函数
         * @param thread 协程执行的线程id,-1标识任意线程
         */
        template <class FiberOrCb>
        void schedule(FiberOrCb fc, int thread = -1)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc, thread);
            }

            if (need_tickle)
            {
                tickle();
            }
        }

        /**
         * @brief 批量调度协程
         *
         * @tparam InputIterator
         * @param begin 协程数组开始位置
         * @param end   协程数组结束位置
         */
        template <class InputIterator>
        void schedule(InputIterator begin, InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                // 遍历
                while (begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin, -1) || need_tickle;
                    ++begin;
                }
            }
            if (need_tickle)
            {
                tickle();
            }
        }

        void switchTo(int thread = -1);
        std::ostream &dump(std::ostream &os);

    protected:
        /**
         * @brief 通知协程调度有任务了
         *
         */
        virtual void tickle();

        /**
         * @brief 协程调度函数
         *
         */
        void run();

        /**
         * @brief 返回是否可以停止
         *
         * @return true
         * @return false
         */
        virtual bool stopping();

        /**
         * @brief 协程无任务可调度时执行idle协程
         *
         */
        virtual void idle();

        /**
         * @brief 设置当前协程调度器
         *
         */
        void setThis();

        /**
         * @brief 是否有空闲线程
         *
         * @return true
         * @return false
         */
        bool hasIdleThreads()
        {
            return m_idleThreadCount > 0;
        }

    private:
        /**
         * @brief 协程调度启动，无锁
         *
         * @tparam FiberOrCb
         * @param fc
         * @param thread
         * @return true
         * @return false
         */
        template <class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc, int thread)
        {
            bool need_tickle = m_fibers.empty();
            FiberAndThread ft(fc, thread);
            if (ft.fiber || ft.cb)
            {
                // 要么是协程，要么是函数指针
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        /**
         * @brief 协程/函数/线程组
         * 定义可以执行的类型
         */
        struct FiberAndThread
        {
            Fiber::ptr fiber;         // 协程
            std::function<void()> cb; // 协程执行函数
            int thread;               // 线程id

            /**
             * @brief Construct a new Fiber And Thread object
             *
             * @param f
             * @param thr
             */
            FiberAndThread(Fiber::ptr f, int thr)
                : fiber(f), thread(thr)
            {
            }

            /**
             * @brief 传入智能指针的指针
             *
             * @param f
             * @param thr
             */
            FiberAndThread(Fiber::ptr *f, int thr)
                : thread(thr)
            {
                fiber.swap(*f);
            }

            FiberAndThread(std::function<void()> f, int thr)
                : cb(f), thread(thr)
            {
            }

            FiberAndThread(std::function<void()> *f, int thr)
                : thread(thr)
            {
                cb.swap(*f);
            }
            // 默认构造函数
            FiberAndThread() : thread(-1)
            {
            }

            /**
             * @brief 重置数据
             *
             */
            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }
        };

    private:
        MutexType m_mutex;                  // 锁
        std::vector<Thread::ptr> m_threads; // 线程池
        std::list<FiberAndThread> m_fibers; // 待执行的任务队列(里面是一个个的协程)
        std::string m_name;                 // 协程调度器名称
        // 为caller线程设计的变量
        Fiber::ptr m_rootFiber; // use_caller=true时，该值为caller线程的调度协程

    protected:
        std::vector<int> m_threadIds;                  // 线程id号数组
        size_t m_threadCount = 0;                      // 线程数量
        std::atomic<size_t> m_activeThreadCount = {0}; // 活跃线程数量（正在干任务的线程数量）
        std::atomic<size_t> m_idleThreadCount = {0};   // 空闲线程数量
        bool m_stopping = true;                        // 是否正在停止
        bool m_autoStop = false;                       // 是否主动停止
        // 为caller线程设计的变量
        int m_rootThread = 0; // use_caller=false时该值为-1; use_caller=true，该值为caller线程id
    };

    class SchedulerSwitcher : public Noncopyable
    {
    public:
        SchedulerSwitcher(Scheduler *target = nullptr);
        ~SchedulerSwitcher();

    private:
        Scheduler *m_caller;
    };

}

#endif