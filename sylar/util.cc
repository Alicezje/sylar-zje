#include "util.h"

namespace sylar
{
    // 先写固定的数
    pid_t GetThreadId()
    {
        return 2048;
    }

    uint32_t GetFiberId()
    {
        return 1024;
    }

    const std::string &GetThreadName()
    {
        static const std::string threadName = "zje_logger";
        return threadName;
    }

}
