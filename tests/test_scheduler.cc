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

int main(int argc, char **argv)
{
    SYLAR_LOG_INFO(g_logger) << "main";
    // sylar::Scheduler sc(3, false, "zje");
    sylar::Scheduler sc(3, true, "zje");

    sc.start();
    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);

    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "over";

    /**
     * 可能存在bug
     * 新建Scheduler对象时，内部调用GetThis方法创建一个主协程，为什么下面还要创建一次？？
     *
     */
    return 0;
}