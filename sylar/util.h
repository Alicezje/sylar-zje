#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <cxxabi.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <iomanip>
#include <iostream>

namespace sylar
{
    /**
     * 返回当前线程的ID
     */
    pid_t GetThreadId();

    /**
     * 返回当前协程的ID
     */
    uint32_t GetFiberId();

    const std::string &GetThreadName();

    /**
     * skip: 
     * 捕获到错误，并知道是从那一层抛出来的
     */
    void Backtrace(std::vector<std::string> &bt, int size=64, int skip = 1);

    /**
     * 将栈的信息转为string
     * size : 想要获取的层数
     * skip : 跳过的层数
     */
    std::string BacktraceToString(int size=64, int skip = 2, const std::string &prefix = "");
}

#endif