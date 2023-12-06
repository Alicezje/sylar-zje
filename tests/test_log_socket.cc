#include <iostream>
#include "log.h"
#include "log_socket.h"

/**
 * @brief 客户端，接收日志进程发来的消息
 *
 * @param argc
 * @param argv
 * @return int
 */

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

    // 定义套接字
    int sockfd;
    // 服务端地址 sockaddr_un 为本地套接字结构体
    struct sockaddr_un servaddr;

    // AF_LOCAL即为使用本地套接字
    sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cout << "create socket failed" << std::endl;
        return -1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    // sun_path为监听的本地文件
    strcpy(servaddr.sun_path, socket_local_path);

    // 建立连接
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        std::cout << "connect failed" << std::endl;
        return -1;
    }

    char send_line[MAXLINE];
    bzero(send_line, MAXLINE);
    char recv_line[MAXLINE];

    // 读取控制台输入
    // while (fgets(send_line, MAXLINE, stdin) != NULL)
    while (true)
    {
        // 获取服务器的消息
        if (read(sockfd, recv_line, MAXLINE) == 0)
        {
            std::cout << "connect failed" << std::endl;
            break;
        }
        // 将recv_line输出到控制台
        std::cout << "receive from log socket";
        fputs(recv_line, stdout);
    }

    return 0;
}