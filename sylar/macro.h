#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include <string.h>
#include <assert.h>
#include "util.h"
#include "log.h"

#if defined __GNUC__ || defined __llvm__
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率成立
#define SYLAR_LIKELY(x) __builtin_expect(!!(x), 1)
/// LIKCLY 宏的封装, 告诉编译器优化,条件大概率不成立
#define SYLAR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LIKELY(x) (x)
#define SYLAR_UNLIKELY(x) (x)
#endif

#define SYLAR_ASSERT(x)                                                               \
    if (!(x))                                                                         \
    {                                                                                 \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION : " #x                        \
                                          << "\nbacktrace:\n"                         \
                                          << sylar::BacktraceToString(10, 2, "    "); \
        std::cout << "\n hhh \n";                                                     \
        assert(x);                                                                    \
    }

#define SYLAR_ASSERT2(x, w)                                                           \
    if (!(x))                                                                         \
    {                                                                                 \
        SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ASSERTION : " #x                        \
                                          << "\n"                                     \
                                          << w                                        \
                                          << "\nbacktrace:\n"                         \
                                          << sylar::BacktraceToString(10, 2, "    "); \
        assert(x);                                                                    \
    }

#endif
