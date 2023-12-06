#include "log.h"
#include "log_socket.h"
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <string>

void temp()
{
    // 定义一个日志器
    sylar::Logger::ptr logger(new sylar::Logger("logger_socket", sylar::LogLevel::DEBUG));

    // 加入控制台输出appender
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    // 添加日志socket作为控制台输出

    std::cout << "enter begin" << std::endl;
    sylar::SocketLogAppender::ptr log_sock{sylar::SocketLogAppender::GetThis()};
    if (log_sock == nullptr)
    {
        SYLAR_LOG_ERROR(logger) << "null";
    }
    // logger->addAppender(sylar::t_socketlog);
    // sylar::SocketLogAppender::GetThis();
    std::cout << "enter end" << std::endl;
}

int main()
{
    // test();
    // test_stu();

    sylar::Logger::ptr logger = SYLAR_LOG_ROOT();

    char send_line[MAXLINE];
    bzero(send_line, MAXLINE);
    // 读取控制台输入
    while (fgets(send_line, MAXLINE, stdin) != NULL)
    {
        SYLAR_LOG_ERROR(logger) << send_line;
    }

    return 0;
}