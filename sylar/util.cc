#include <execinfo.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "util.h"
#include "log.h"
#include "fiber.h"

namespace sylar
{
    sylar::Logger::ptr g_util_logger = SYLAR_LOG_NAME("system");

    // 先写固定的数
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    uint32_t GetFiberId()
    {
        return sylar::Fiber::GetFiberId();
        // return 1024;
    }

    const std::string &GetThreadName()
    {
        static const std::string threadName = "zje_logger";
        return threadName;
    }

    void Backtrace(std::vector<std::string> &bt, int size, int skip)
    {
        void **array = (void **)malloc((sizeof(void *) * size));
        // 获取实际栈的深度吗？
        size_t s = ::backtrace(array, size);

        char **strings = backtrace_symbols(array, s);
        if (strings == NULL)
        {
            SYLAR_LOG_ERROR(g_util_logger) << "backtrace_synbols error";
            free(array);
            return;
        }
        for (size_t i = skip; i < s; i++)
        {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size, int skip, const std::string &prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt, size, skip);
        std::stringstream ss;
        for (size_t i = 0; i < bt.size(); i++)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

}
