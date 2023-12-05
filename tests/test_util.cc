#include <iostream>
#include <assert.h>
#include "util.h"
#include "log.h"
#include "macro.h"

void test_assert()
{
    sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
    SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
    SYLAR_ASSERT(0 == 1);
}

int main()
{
    std::cout << "hello" << std::endl;
    test_assert();
    std::cout << "end" << std::endl;
    return 0;
}