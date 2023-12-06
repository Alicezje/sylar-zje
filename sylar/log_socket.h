#ifndef __SYLAR_LOG_SOCKET_H__
#define __SYLAR_LOG_SOCKET_H__

#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <string.h>

#define MAXLINE 4096
#define LISTENQ 1024
#define BUFFER_SIZE 4096

namespace sylar
{
    // 通过socket输入其他进程,使用本地套接字实现
    class SocketLogAppender : public LogAppender, public std::enable_shared_from_this<SocketLogAppender>
    {
    public:
        static SocketLogAppender *t_socketlog;

    public:
        typedef std::shared_ptr<SocketLogAppender> ptr;

        SocketLogAppender()
        {
        }

        SocketLogAppender(char *socket_local_path);
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

        /**
         * @brief 初始化套接字
         *
         */
        void init();

        std::string toYamlString() override;

        ~SocketLogAppender()
        {
            // 关闭连接
            close(connfd);
            close(listenfd);
        }

    public:
        static void SetThis(SocketLogAppender *s);

        static SocketLogAppender::ptr GetThis();

    private:
        int listenfd;                  // 监听套接字
        int connfd;                    // 连接套接字
        struct sockaddr_un servaddr;   // 服务端套接字结构体
        struct sockaddr_un clientaddr; // 客服端套接字结构体
        char *m_socket_local_path;     // 监听本地套接字文件 socket_local_file

        // 日志内容,调用format会返回一个字符串，将该字符串发给线程
        std::stringstream send_line; // 发送缓冲区
        bool is_connect = false;     // 是否已经有客户端连接
    };
}

#endif