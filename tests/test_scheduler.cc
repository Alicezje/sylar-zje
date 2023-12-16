#include <iostream>
#include "log.h"
#include "scheduler.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber()
{
    static int s_count = 5;
    SYLAR_LOG_INFO(g_logger) << "test in fiber s_count=" << s_count;

    sleep(1);
    if (--s_count >= 0)
    {
        // 添加调度任务
        sylar::Scheduler::GetThis()->schedule(&test_fiber, sylar::GetThreadId());
    }
}

void test_fiber_1()
{
    SYLAR_LOG_INFO(g_logger) << "test in fiber";
}

int main(int argc, char **argv)
{
    SYLAR_LOG_INFO(g_logger) << "main";
    sylar::Scheduler sc(1, false, "zje");
    // sylar::Scheduler sc(3, true, "zje");

    sc.start();

    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "schedule";

    // 加入任务，可以执行该任务放到执行线程上执行
    // sc.schedule(&test_fiber);
    sc.schedule(&test_fiber_1);

    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";

    return 0;
}