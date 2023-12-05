#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include <atomic>

namespace sylar
{
    sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id{0};
    static std::atomic<uint64_t> s_fiber_count{0};

    /**
     * 线程局部变量
     * 当前线程正在运行的协程
     */
    static thread_local Fiber *t_fiber = nullptr;
    /**
     * 主协程
     */
    static thread_local Fiber::ptr t_threadFiber = nullptr;

    // 配置协程栈的大小
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size =
        Config::Lookup<uint32_t>("fiber.stack_size", 1024 * 1024, "fiber stack size");

    // 栈内存分配
    class MallocStackAllocator
    {
    public:
        static void *Alloc(size_t size)
        {
            return malloc(size);
        }

        static void Dealloc(void *vp, size_t size)
        {
            return free(vp);
        }
    };

    // 如果想切换到其他分配器，可直接在这里切换
    using StackAllocator = MallocStackAllocator;

    Fiber::Fiber()
    {
        m_state = EXEC; // 设置为执行态
        SetThis(this);  // main协程，将自己放进去

        if (getcontext(&m_cxt))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }

        // 主协程没有栈

        // 协程数+1
        ++s_fiber_count;

        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() create main fiber";
    }

    Fiber::Fiber(std::function<void()> cb, size_t stacksize)
        : m_id(++s_fiber_id),
          m_cb(cb)
    {
        ++s_fiber_count;
        // 设置栈大小
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();
        // 申请栈内存
        m_stack = StackAllocator::Alloc(m_stacksize);
        // 获取上下文
        if (getcontext(&m_cxt))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        // 设置上下文信息
        m_cxt.uc_link = nullptr;                  // 关联上下文
        m_cxt.uc_stack.ss_sp = m_stack;           // 栈指针
        m_cxt.uc_stack.ss_size = m_stacksize;     // 栈大小
        makecontext(&m_cxt, &Fiber::MainFunc, 0); // 创建一个新的上下文(函数赋值到上下文里)

        // 初始状态？？ 默认为INIT
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber create sub fiber, id= " << m_id;
    }

    Fiber::~Fiber()
    {
        --s_fiber_count;
        if (m_stack)
        {
            SYLAR_ASSERT(m_state == TERM ||
                         m_state == EXCEPT ||
                         m_state == INIT);
            // 回收栈
            StackAllocator::Dealloc(m_stack, m_stacksize);
        }
        else
        {
            // 如果m_stack为空，说明为主协程
            SYLAR_ASSERT(!m_cb);           // 主协程无m_cn
            SYLAR_ASSERT(m_state == EXEC); // 仍为EXEC状态

            Fiber *cur = t_fiber;
            if (cur == this)
            {
                // 当前运行协程为主协程,置为null
                SetThis(nullptr);
            }
        }
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id=" << m_id
                                  << " total=" << s_fiber_count;
    }

    void Fiber::reset(std::function<void()> cb)
    {
        // 为了充分利用内存，一个协程执行完了，但分配的内存可以重复使用
        SYLAR_ASSERT(m_stack);
        SYLAR_ASSERT(m_state == TERM ||
                     m_state == EXCEPT ||
                     m_state == INIT);

        m_cb = cb;

        if (getcontext(&m_cxt))
        {
            SYLAR_ASSERT2(false, "getcontext");
        }
        // 设置上下文信息
        m_cxt.uc_link = nullptr;              // 关联上下文
        m_cxt.uc_stack.ss_sp = m_stack;       // 栈指针
        m_cxt.uc_stack.ss_size = m_stacksize; // 栈大小

        makecontext(&m_cxt, &Fiber::MainFunc, 0);
        m_state = INIT;
    }

    void Fiber::SetThis(Fiber *f)
    {
        t_fiber = f;
    }

    Fiber::ptr Fiber::GetThis()
    {
        if (t_fiber)
        {
            // 如果当前运行协程不为空，则直接返回
            return t_fiber->shared_from_this();
        }
        // 如果没有就创建一个主协程,使用智能指针管理
        Fiber::ptr main_fiber(new Fiber); // 调用无参构造函数，内部将创建好的协程赋给t_fiber
        SYLAR_ASSERT(t_fiber == main_fiber.get());
        // 赋值给主协程
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

    void Fiber::swapIn()
    {
        // 正在运行的协程放到后台，执行当前协程
        SetThis(this);

        SYLAR_ASSERT(m_state != EXEC);
        m_state = EXEC; // 设为运行态
        // 更改上下文: 恢复m_cxt执行的上下文，将当前上下文存入主协程中
        if (swapcontext(&t_threadFiber->m_cxt, &m_cxt))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
        // 主协程状态设为什么？主协程一直为EXEC状态
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::swapIn() to id=" << m_id;
    }

    void Fiber::swapOut()
    {
        // 获取主协程，设置到主协程
        SetThis(t_threadFiber.get());
        // 交换上下文
        if (swapcontext(&m_cxt, &t_threadFiber->m_cxt))
        {
            SYLAR_ASSERT2(false, "swapcontext");
        }
        // 状态设为什么？HOLD 还是 READY
        SYLAR_LOG_DEBUG(g_logger) << "Fiber::swapOut() from id=" << m_id;
    }

    void Fiber::YieldToReady()
    {
        // 获得当前协程
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = READY;
        cur->swapOut();
    }

    void Fiber::YieldToHold()
    {
        // 获得当前协程
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur->m_state == EXEC);
        cur->m_state = HOLD;
        cur->swapOut();
    }

    uint64_t Fiber::TotalFibers()
    {
        return s_fiber_count;
    }

    void Fiber::MainFunc()
    {
        // 获得当前执行的协程
        Fiber::ptr cur = GetThis();
        SYLAR_ASSERT(cur);
        try
        {
            // 调用绑定的函数
            cur->m_cb();
            // 执行完后将函数置为nullptr
            cur->m_cb = nullptr;
            // 设置协程状态为TERM，结束态
            cur->m_state = TERM;
        }
        catch (std::exception &ex)
        {
            // 出现异常
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << " Fiber Except " << ex.what();
        }
        catch (...)
        {
            cur->m_state = EXCEPT;
            SYLAR_LOG_ERROR(g_logger) << " Fiber Except ";
        }

        auto raw_ptr = cur.get(); // 获得当前协程
        cur.reset();              // 重置智能指针
        raw_ptr->swapOut();       // 切换到主线程，保证协程执行完可以回到主协程

        SYLAR_ASSERT2(false, "never reach");
    }

    uint64_t Fiber::GetFiberId()
    {
        if (t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }
}