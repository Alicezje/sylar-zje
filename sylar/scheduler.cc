#include "scheduler.h"
#include "log.h"
#include "macro.h"

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    // 当前线程的协程调度器，同一个调度器下所有线程的指向同一个调度器实例
    static thread_local Scheduler *t_scheduler = nullptr;
    // 调度器中当前线程的调度协程
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
            // m_rootFiber为caller线程中的调度协程，run方法运行在协程里
            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true)); // caller线程的调度协程

            sylar::Thread::SetName(m_name);
            // 调度器中当前线程的调度协程
            t_scheduler_fiber = m_rootFiber.get();

            // 获取当前线程ID
            m_rootThread = sylar::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            // 不使用caller线程进行调度，则将m_rootThread置为-1，标记一下
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
            // 当stopping()为false，也就是调度还没停止，还有活跃的线程，将idle进程挂起，状态转为Hold
            sylar::Fiber::YieldToHold();
        }
        // 当stopping()为true，没有任务要执行了，则idel一直执行(配合run函数内的while循环中swapIn())
    }

    /**D
     * @brief 判断调度器是否可以停止
     * 当所有任务执行完了，调度器才可以停止
     * @return true
     * @return false
     */
    bool Scheduler::stopping()
    {
        // SYLAR_LOG_INFO(g_logger) << "stopping";
        MutexType::Lock lock(m_mutex);
        /**
         * 自动停止
         * & 已经停止
         * & 待执行协程队列为空
         * & 活动线程数量为0
         * 则 返回true,代表没有任务要执行了
         *
         */
        return m_autoStop && m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::start()
    {
        // 线程池要启动线程
        MutexType::Lock lock(m_mutex);
        // m_stopping初始值为true,是否不是正在停止
        if (!m_stopping)
        {
            // 正在停止，则返回
            return;
        }
        // 置为未停止...
        m_stopping = false;
        SYLAR_ASSERT(m_threads.empty());

        // 分配对应数量的线程指针
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // 向线程池里放数据,线程的执行函数该Scheduler的run方法(run方法运行在协程里)
            // new Thread()方法内部会创建一个线程执行
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        SYLAR_LOG_INFO(g_logger) << "Scheduler::start() end thread num :" << m_threadIds.size();
        lock.unlock();
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

        if (m_rootThread != -1)
        {
            // 未使用caller线程
            SYLAR_ASSERT(GetThis() == this);
        }
        else
        {
            // 使用caller线程
            SYLAR_ASSERT(GetThis() != this);
        }

        m_stopping = true;
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // 通知还有任务，其实啥也没做
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            // 如果调度器只使用了caller线程来调度，那caller调度器真正开始执行调度的位置就是这个stop方法
            // 如果使用了caller线程，需要将caller线程再执行一下
            if (!stopping())
            {
                m_rootFiber->call();
            }
        }

        std::vector<Thread::ptr> thrs;
        {
            // 遍历线程池
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            // 阻塞等待所有线程执行完毕，才能停止
            i->join();
        }
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

        setThis(); // t_scheduler = this;

        if (sylar::GetThreadId() != m_rootThread)
        {
            // m_rootThread记录的是使用caller线程进行调度时的线程id
            // 当前线程如果不是caller线程，需要获取当前线程的调度线程
            SYLAR_LOG_DEBUG(g_logger) << "run(): sylar::GetThreadId() != m_rootThread";

            // 内部如果没有主协程，则会创建一个主协程
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        // 没有任务时执行的协程
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
            bool is_active = false; // 标记是否找到任务放入线程中执行
            // 从协程的消息队列里取出一个协程赋给ft
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                // 循环遍历所有协程
                while (it != m_fibers.end())
                {
                    if (it->thread != -1 && it->thread != sylar::GetThreadId())
                    {
                        // 该协程所在线程id!=-1(未使用caller进行协程调度) && 当前线程 != 该协程所在线程
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
                    ++m_activeThreadCount; // 活跃线程数量+1
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
                // 唤醒该协程，进行执行(与t_scheduler_fiber进行切换)
                ft.fiber->swapIn();
                // 执行swapOut后才能回到这里，活跃线程数-1
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
                    // 重置线程，用于复用，状态设为INIT
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }

                // cb_fiber中存放的是要执行的方法

                ft.reset();
                cb_fiber->swapIn(); // 切换到cb_fiber执行
                // 执行结束或者swapOut才会回到这里，活跃线程数-1
                --m_activeThreadCount;

                if (cb_fiber->getState() == Fiber::READY)
                {
                    // 未执行完毕，重新加入到任务队列中
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
                // 该线程未找到可以执行的ft

                if (is_active)
                {
                    // 防止找到了fiber，但是不满足上边的条件，所以这里要将活动线程数-1
                    --m_activeThreadCount;
                    continue;
                }

                if (idle_fiber->getState() == Fiber::TERM) // idle的状态为结束TERM
                {
                    // 执行结束，while循环的唯一退出条件
                    SYLAR_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                // 没有任务可以执行，则执行idle协程
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