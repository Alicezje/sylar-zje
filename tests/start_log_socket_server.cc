#include "log.h"
#include "log_socket.h"
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>

void server_fun_1(char *socket_local_path)
{
    sylar::Logger::ptr logger = SYLAR_LOG_ROOT();
    // 加入控制台输出appender
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    // 定义logsocket输出,挂到appender中
    sylar::SocketLogAppender::ptr log_socket{new sylar::SocketLogAppender(socket_local_path)};
    logger->addAppender(log_socket);

    // 开启服务，阻塞等待连接
    log_socket->init();

    while (true)
    {
        sleep(3);
        SYLAR_LOG_ERROR(logger) << "hello";
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "usage: unixstreamserver <local_path>" << std::endl;
        return -1;
    }
    // 获取m_socket_local_path
    char *socket_local_path = argv[1];
    std::cout << socket_local_path << std::endl;

    server_fun_1(socket_local_path);
    std::cout<<"zje"<<std::endl;
    return 0;
}