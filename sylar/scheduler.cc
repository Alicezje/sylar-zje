#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    // 协程调度器
    static thread_local Scheduler *t_scheduler = nullptr;
    // 调度器中的主协程
    static thread_local Fiber *t_scheduler_fiber = nullptr;

    Scheduler::Scheduler(size_t threads, bool use_caller, const std::string &name)
        : m_name(name)
    {
        SYLAR_ASSERT(threads > 0);
        // use_caller 是否使用当前调用线程

        // 是否将当前的线程加入调度？
        if (use_caller)
        {
            // 如果主协程不存在，内部会创建主协程
            sylar::Fiber::GetThis();
            // 线程池可用数量-1
            --threads;

            SYLAR_ASSERT(GetThis() == nullptr);
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // 这样不就两个主协程了吗？

            sylar::Thread::SetName(m_name);

            t_scheduler_fiber = m_rootFiber.get();

            // 获取当前线程ID
            m_rootThread = sylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        SYLAR_ASSERT(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }
    Fiber *Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    void Scheduler::idle()
    {
        SYLAR_LOG_INFO(g_logger) << "idle function";
        while (!stopping())
        {
            // 当stopping()为false，也就是调度还没停止，将idle进程挂起，状态转为Hold
            sylar::Fiber::YieldToHold();
        }
        // 当stopping()为true，也就是调度已经结束，则Fiber内部会将状态置为TERM
    }

    bool Scheduler::stopping()
    {
        // SYLAR_LOG_INFO(g_logger) << "stopping";
        MutexType::Lock lock(m_mutex);
        /**
         * 自动停止
         * & 已经停止
         * & 待执行协程队列为空
         * & 活动线程数量为0
         * 则 返回true,代表没有任务要执行了，调度结束
         *
         */
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::start()
    {
        // 线程池要启动线程
        MutexType::Lock lock(m_mutex);
        // 代表什么意思？m_stopping初始值为true
        if (!m_stopping)
        {
            // 正在停止，则返回
            return;
        }
        // 置为不是正在停止
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());

        // 分配对应数量的线程指针
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // 向线程池里放数据,线程的执行函数该Scheduler的run方法
            // new Thread()方法内部会创建一个线程执行
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        SYLAR_LOG_INFO(g_logger) << "Scheduler::start() end thread num :" << m_threadIds.size();
        lock.unlock();

        // if (m_rootFiber)
        // {
        //     m_rootFiber->swapIn();
        // }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            SYLAR_LOG_INFO(g_logger) << this << " stopped";
            m_stopping = true;

            if (stopping())
            {
                return;
            }
        }

        /**
         * stop分两种情况
         * scheduler用了use_caller,用来一定要在scheduler创建的那个线程里执行
         * scheduler没用use_caller
         */
        // bool exit_on_this_fiber = false;
        if (m_rootThread != -1)
        {
            SYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            SYLAR_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            // while(!stopping()) {
            //     if(m_rootFiber->getState() == Fiber::TERM
            //             || m_rootFiber->getState() == Fiber::EXCEPT) {
            //         m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
            //         SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
            //         t_fiber = m_rootFiber.get();
            //     }
            //     m_rootFiber->call();
            // }
            if (!stopping())
            {
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            i->join();
        }
        // if(exit_on_this_fiber) {
        // }
    }

    void Scheduler::tickle()
    {
        // 通知协程调度有任务了
        SYLAR_LOG_INFO(g_logger) << "tickle";
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    void Scheduler::run()
    {
        /**
         * @brief
         * 协程消息队列里面是否有任务
         * 无任务执行，执行idle
         */

        SYLAR_LOG_DEBUG(g_logger) << m_name << " run";
        // set_hook_enable(true);
        // Fiber::GetThis();

        // 将协程调度器设为当前该对象？？
        setThis();
        if (sylar::GetThreadId() != m_rootThread)
        {
            SYLAR_LOG_DEBUG(g_logger) << "run(): sylar::GetThreadId() != m_rootThread";
            // 当前协程不等于主协程，则需要切换到当前线程的主协程上
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        // 无用协程？
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));

        if (m_fibers.empty())
        {
            SYLAR_LOG_DEBUG(g_logger) << "run(): m_fiber is null";
        }

        // for (auto i = m_fibers.begin(); i != m_fibers.end(); i++)
        // {
        //     std::cout << (i->fiber)->getState() << std::endl;
        // }

        Fiber::ptr cb_fiber;
        FiberAndThread ft;
        // 此时已经在当前线程的主协程上了
        while (true)
        {
            // 一直循环调度
            // SYLAR_LOG_DEBUG(g_logger) << "enter while";
            ft.reset();
            bool tickle_me = false; // 标注是否还有任务未执行
            bool is_active = false;
            // 从协程的消息队列里取出一个协程赋给ft
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                // 循环遍历所有协程
                while (it != m_fibers.end())
                {
                    if (it->thread != -1 && it->thread != sylar::GetThreadId())
                    {
                        // 该协程所在线程id!=-1 && 当前线程 != 该协程所在线程
                        // 自己不能处理要交给其他线程处理
                        ++it; // 向下遍历下一个协程
                        tickle_me = true;
                        continue;
                    }

                    SYLAR_ASSERT(it->fiber || it->cb);
                    // fiber正在执行状态,无须执行
                    if (it->fiber && it->fiber->getState() == Fiber::EXEC)
                    {
                        ++it;
                        continue;
                    }
                    // 获取该协程赋给ft,并在协程处理队列m_fibers中移除元素it
                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    is_active = true;
                    // 找到一个可以执行的协程，则退出
                    break;
                }
                tickle_me |= it != m_fibers.end();
            }

            if (tickle_me)
            {
                // 唤醒其他线程
                tickle();
            }

            // SYLAR_ASSERT(ft.fiber != nullptr);
            // SYLAR_ASSERT((ft.cb != nullptr || ft.fiber != nullptr));
            // 可以执行的协程体为ft
            if (ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
            {
                // SYLAR_LOG_DEBUG(g_logger) << "ft.fiber is not nullptr";

                // 执行的是fiber，然后该fiber为结束&&未出现异常
                // ++m_activeThreadCount;
                // 唤醒该协程，进行执行
                ft.fiber->swapIn();
                --m_activeThreadCount;

                // 执行完成后
                if (ft.fiber->getState() == Fiber::READY)
                {
                    // 未执行结束，还要去执行,则调用schedule方法将该协程重新放入协程队列m_fibers中
                    schedule(ft.fiber);
                }
                else if (ft.fiber->getState() != Fiber::TERM &&
                         ft.fiber->getState() != Fiber::EXCEPT)
                {
                    // 让出执行事件,进入hold状态
                    ft.fiber->setState(Fiber::HOLD);
                }
            }
            else if (ft.cb) // 可执行体为函数
            {
                // SYLAR_LOG_DEBUG(g_logger) << "ft.cb is not nullptr";

                // 执行函数
                if (cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->swapIn();
                --m_activeThreadCount;
                if (cb_fiber->getState() == Fiber::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if (cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                { // if(cb_fiber->getState() != Fiber::TERM) {
                    cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else
            {
                // SYLAR_LOG_DEBUG(g_logger) << "ft is nullptr";

                if (is_active)
                {
                    --m_activeThreadCount;
                    continue;
                }
                if (idle_fiber->getState() == Fiber::TERM)
                {
                    // 执行结束，while循环的唯一退出条件
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                // 执行idle协程
                idle_fiber->swapIn();
                --m_idleThreadCount;
                if (idle_fiber->getState() != Fiber::TERM && idle_fiber->getState() != Fiber::EXCEPT)
                {
                    idle_fiber->m_state = Fiber::HOLD;
                }
            }
        }
    }

    void Scheduler::switchTo(int thread)
    {
        SYLAR_ASSERT(Scheduler::GetThis() != nullptr);
        if (Scheduler::GetThis() == this)
        {
            if (thread == -1 || thread == sylar::GetThreadId())
            {
                return;
            }
        }
        schedule(Fiber::GetThis(), thread);
        Fiber::YieldToHold();
    }

    std::ostream &Scheduler::dump(std::ostream &os)
    {
        os << "[Scheduler name=" << m_name
           << " size=" << m_threadCount
           << " active_count=" << m_activeThreadCount
           << " idle_count=" << m_idleThreadCount
           << " stopping=" << m_stopping
           << " ]" << std::endl
           << "    ";
        for (size_t i = 0; i < m_threadIds.size(); ++i)
        {
            if (i)
            {
                os << ", ";
            }
            os << m_threadIds[i];
        }
        return os;
    }

    SchedulerSwitcher::SchedulerSwitcher(Scheduler *target)
    {
        m_caller = Scheduler::GetThis();
        if (target)
        {
            target->switchTo();
        }
    }

    SchedulerSwitcher::~SchedulerSwitcher()
    {
        if (m_caller)
        {
            m_caller->switchTo();
        }
    }

}