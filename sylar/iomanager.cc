#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

namespace sylar
{
    static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

    enum EpollCtlOp
    {
    };

    static std::ostream &operator<<(std::ostream &os, const EpollCtlOp &op)
    {
        switch ((int)op)
        {
#define XX(ctl) \
    case ctl:   \
        return os << #ctl;
            XX(EPOLL_CTL_ADD);
            XX(EPOLL_CTL_MOD);
            XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
        }
#undef XX
    }

    static std::ostream &operator<<(std::ostream &os, EPOLL_EVENTS events)
    {
        if (!events)
        {
            return os << "0";
        }
        bool first = true;
#define XX(E)          \
    if (events & E)    \
    {                  \
        if (!first)    \
        {              \
            os << "|"; \
        }              \
        os << #E;      \
        first = false; \
    }
        XX(EPOLLIN);
        XX(EPOLLPRI);
        XX(EPOLLOUT);
        XX(EPOLLRDNORM);
        XX(EPOLLRDBAND);
        XX(EPOLLWRNORM);
        XX(EPOLLWRBAND);
        XX(EPOLLMSG);
        XX(EPOLLERR);
        XX(EPOLLHUP);
        XX(EPOLLRDHUP);
        XX(EPOLLONESHOT);
        XX(EPOLLET);
#undef XX
        return os;
    }

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::READ: // 返回读事件上下文
            return read;
        case IOManager::WRITE:
            return write; // 返回写事件上下文
        default:
            SYLAR_ASSERT2(false, "getContext");
        }
        throw std::invalid_argument("getContext invalid event");
    }

    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }

    void IOManager::FdContext::triggerEvent(IOManager::Event event)
    {
        // 触发事件,即将该事件对应的函数体加入到调度器中执行

        // SYLAR_LOG_INFO(g_logger) << "fd=" << fd
        //     << " triggerEvent event=" << event
        //     << " events=" << events;
        SYLAR_ASSERT(events & event);
        // if(SYLAR_UNLIKELY(!(event & event))) {
        //     return;
        // }

        // events结果为0000,即当前的事件记为None,这一步是为了干什么？将当前事件初始化为NONE
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            // 函数不为空，将函数加入到调度队列中
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            // 协程不为空，将协程加入到调度队列中
            ctx.scheduler->schedule(&ctx.fiber);
        }
        ctx.scheduler = nullptr;
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name)
        : Scheduler(threads, use_caller, name)
    {
        // IO协程调度器构造函数
        // 创建epoll
        m_epfd = epoll_create(5000);
        SYLAR_ASSERT(m_epfd > 0);
        // 创建pipe, bytes written on PIPEDES[1] can be read from PIPEDES[0].
        int rt = pipe(m_tickleFds);
        SYLAR_ASSERT(!rt);

        // 创建读事件，监听第一个句柄
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET; // 读事件，边缘触发
        event.data.fd = m_tickleFds[0];

        // 将其设置为非阻塞
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        SYLAR_ASSERT(!rt);

        // 添加第一个文件句柄到epoll实例中
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        SYLAR_ASSERT(!rt);

        contextResize(32);
        // 开始调度
        start();
    }

    IOManager::~IOManager()
    {
        // 停止调度
        stop();
        // 关闭文件描述符
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        // 释放内存
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                // 如果不存在就新建一个，下标就是fd的值
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        // socket事件上下文FdContext <fd,event,cb>
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            // m_fdContexts中下标即为该上下文
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            // 容量不够，需要扩容
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5);
            fd_ctx = m_fdContexts[fd];
        }

        // 加锁
        FdContext::MutexType::Lock lock2(fd_ctx->mutex);

        // 0000 & event = 0000
        if (SYLAR_UNLIKELY(fd_ctx->events & event))
        {
            SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                                      << " event=" << (EPOLL_EVENTS)event
                                      << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            SYLAR_ASSERT(!(fd_ctx->events & event));
        }
        // 如果该事件已经存在，需要进行修改操作，如果不存在进行添加操作
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;

        // 定义epoll_event
        epoll_event epevent;
        // 边缘触发 | 事件
        epevent.events = EPOLLET | fd_ctx->events | event;
        // 数据类设置为socket事件上下文类
        epevent.data.ptr = fd_ctx;

        // 对epoll进行操作
        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
                                      << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        // 待执行事件+1
        ++m_pendingEventCount;
        // 修改socket事件上下文类中的events值
        fd_ctx->events = (Event)(fd_ctx->events | event);
        // 获取对应事件：read | write , 获取后设置<scheduler,fiber,cb>
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        // 设置调度器
        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            // 执行函数不为空，设置执行函数
            event_ctx.cb.swap(cb);
        }
        else
        {
            // 获得当前正在运行的协程,直接将该事件交给当前协程来执行吗？
            event_ctx.fiber = Fiber::GetThis();
            SYLAR_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        // 删除socket上下文事件
        // 根据fd获取到该事件，

        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        // 获取到该事件
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (SYLAR_UNLIKELY(!(fd_ctx->events & event)))
        {
            return false;
        }
        // 一般来说，fd_ctx->events和event值相同，所以new_events结果为0
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        fd_ctx->events = new_events;

        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    bool IOManager::cancelEvent(int fd, Event event)
    {
        // 去掉事件
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (SYLAR_UNLIKELY(!(fd_ctx->events & event)))
        {
            return false;
        }

        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        // 触发执行它
        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!fd_ctx->events)
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                      << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << " (" << errno << ") (" << strerror(errno) << ")";
            return false;
        }
        // 触发执行所有事件
        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        SYLAR_ASSERT(fd_ctx->events == 0);
        return true;
    }

    IOManager *IOManager::GetThis()
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            // 如果没有闲置的线程，则不需要去通知了
            return;
        }
        // 向管道pipe[1]中写入1个字节，值为T
        int rt = write(m_tickleFds[1], "T", 1);
        SYLAR_ASSERT(rt == 1);
    }

    bool IOManager::stopping(uint64_t &timeout)
    {
        // timeout = getNextTimer();
        // return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
        // 可以停止且待处理事件为0
        return Scheduler::stopping() && m_pendingEventCount == 0;
    }

    bool IOManager::stopping()
    {
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    void IOManager::idle()
    {
        /**
         * 当线程什么事情都不干时,就会陷入idle协程
         * 处理一些相关事情，没有事情做则陷入epoll_wait
         * 在epoll_wait中检测到就绪文件描述符，则将其加入到调度器中
         * 
         */
        SYLAR_LOG_DEBUG(g_logger) << "idle";

        const uint64_t MAX_EVNETS = 256;
        epoll_event *events = new epoll_event[MAX_EVNETS](); // 建立epoll_event数组，拷贝到内核中
        // 用智能指针的方式管理数组，超出作用域后进行析构，将数组释放
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                                   { delete[] ptr; });

        while (true)
        {
            uint64_t next_timeout = 0;
            if (SYLAR_UNLIKELY(stopping(next_timeout)))
            {
                // 已经结束离开循环
                SYLAR_LOG_INFO(g_logger) << "name=" << getName()
                                         << " idle stopping exit";
                break;
            }

            int rt = 0;
            do
            {
                static const int MAX_TIMEOUT = 3000; // 超时事件，避免忙等
                if (next_timeout != ~0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT
                                       ? MAX_TIMEOUT
                                       : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                // 获取事件是否有触发的
                /**
                 * next_timeout
                 * 0: 函数不阻塞，不管epoll实例中有没有就绪的文件描述符，函数被调用后都直接返回
                 * 大于0: 如果epoll实例中没有已就绪的文件描述符，函数阻塞对应的毫秒数再返回
                 * 小于0: 函数一直阻塞，直到epoll实例中有已就绪的文件描述符之后才解除阻塞
                 */
                rt = epoll_wait(m_epfd, events, MAX_EVNETS, (int)next_timeout);
                /**
                 * rt
                 * 等于0: 函数是阻塞被强制解除了, 没有检测到满足条件的文件描述符
                 * 大于0: 检测到的已就绪的文件描述符的总个数
                 * -1: 失败
                 */
                if (rt < 0 && errno == EINTR)
                {
                    // 失败
                }
                else
                {
                    // 如果有，则退出
                    break;
                }
            } while (true);

            std::vector<std::function<void()>> cbs;
            // listExpiredCb(cbs);
            if (!cbs.empty())
            {
                // SYLAR_LOG_DEBUG(g_logger) << "on timer cbs.size=" << cbs.size();
                // 假如到调度队列中
                schedule(cbs.begin(), cbs.end());
                cbs.clear();
            }

            // if(SYLAR_UNLIKELY(rt == MAX_EVNETS)) {
            //     SYLAR_LOG_INFO(g_logger) << "epoll wait events=" << rt;
            // }

            // 遍历处理所有就绪的事件
            for (int i = 0; i < rt; ++i)
            {
                // 获取当前epoll事件（内部数据域data中存放的fd）
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0])
                {
                    // 读事件
                    // 外部有发消息所以唤醒了
                    uint8_t dummy[256];
                    // 循环读取消息，因为使用边缘触发模式，如果不读完，下次就不会通知了
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                        ;
                    continue;
                }
                // 写事件
                // 从数据域中取出FdContext，事件的上下文类
                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                // 加锁
                FdContext::MutexType::Lock lock(fd_ctx->mutex);
                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    // 如果发生错误，或者中断，需要修改event
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                int real_events = NONE;

                // 读
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }

                // 写
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                if ((fd_ctx->events & real_events) == NONE)
                {
                    // 无事件，可能被其他的处理掉了
                    continue;
                }

                int left_events = (fd_ctx->events & ~real_events); // 剩余事件 = 当前事件减掉初始事件
                // 如果left_events不为0,则需要修改事件,否则需要删除
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                // 复用event事件
                event.events = EPOLLET | left_events;
                // 对epoll里进行修改
                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                                              << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                              << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
                //                          << " real_events=" << real_events;
                if (real_events & READ)
                {
                    // 触发读事件
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    // 触发写事件
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            // 让出执行权
            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            raw_ptr->swapOut();
        }
    }

    // void IOManager::onTimerInsertedAtFront()
    // {
    //     tickle();
    // }

}
