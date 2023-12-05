#include "fiber.h"
#include "log.h"
#include <iostream>

sylar::Logger::ptr g_fiber_logger = SYLAR_LOG_ROOT();

void run_in_fiber()
{
    SYLAR_LOG_INFO(g_fiber_logger) << "run_in_fiber begin";
    sylar::Fiber::YieldToHold();
    SYLAR_LOG_INFO(g_fiber_logger) << "run_in_fiber end";
    sylar::Fiber::YieldToReady();
}

void test_fiber()
{
    SYLAR_LOG_INFO(g_fiber_logger) << "main begin -1";
    {
        // 一个线程开始创建协程前都要调用GetThis方法
        sylar::Fiber::GetThis();
        sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
        SYLAR_LOG_INFO(g_fiber_logger) << "main begin";
        fiber->swapIn();
        SYLAR_LOG_INFO(g_fiber_logger) << "main after swapin";
        fiber->swapIn();
    }
    SYLAR_LOG_INFO(g_fiber_logger) << "main after end";
}

// int main(int argc, char **argv)
// {
//     sylar::Thread::SetName("main");

//     std::vector<sylar::Thread::ptr> thrs;
//     // 多线程多协程
//     for (int i = 0; i < 3; i++)
//     {
//         thrs.push_back(sylar::Thread::ptr(new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
//     }

//     for (auto i : thrs)
//     {
//         i->join();
//     }
//     return 0;
// }

int main(int argc, char **argv)
{
    sylar::Thread::SetName("main");
    SYLAR_LOG_INFO(g_fiber_logger) << "main begin -1";
    {
        sylar::Fiber::GetThis();
        // 创建一个协程a
        sylar::Fiber::ptr fiber_a(new sylar::Fiber(run_in_fiber));
        SYLAR_LOG_INFO(g_fiber_logger) << "main begin";
        // 切换到协程a
        fiber_a->swapIn();
        SYLAR_LOG_INFO(g_fiber_logger) << "main after swapin";
        // 切换到协程a
        fiber_a->swapIn();
        SYLAR_LOG_INFO(g_fiber_logger) << "total: " << sylar::Fiber::TotalFibers();
    }

    SYLAR_LOG_INFO(g_fiber_logger) << "main after end";
    return 0;
}