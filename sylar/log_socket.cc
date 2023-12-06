#include "log_socket.h"

namespace sylar
{
    // static SocketLogAppender *t_socketlog; // 存放日志套接字的指针
    SocketLogAppender *SocketLogAppender::t_socketlog = nullptr;

    SocketLogAppender::SocketLogAppender(char *socket_local_path)
    {
        m_socket_local_path = socket_local_path;
    }

    void SocketLogAppender::SetThis(SocketLogAppender *s)
    {
        t_socketlog = s;
    }

    SocketLogAppender::ptr SocketLogAppender::GetThis()
    {
        return SocketLogAppender::ptr(t_socketlog);
    }

    void SocketLogAppender::init()
    {
        listenfd = socket(AF_LOCAL, SOCK_STREAM, 0);
        if (listenfd < 0)
        {
            std::cout << "socket create failed" << std::endl;
            return;
        }
        // 将本地文件删除
        unlink(m_socket_local_path);
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sun_family = AF_LOCAL;
        strcpy(servaddr.sun_path, m_socket_local_path);

        // 绑定
        if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        {
            std::cout << "bind failed" << std::endl;
            return;
        }

        // 监听
        if (listen(listenfd, LISTENQ) < 0)
        {
            std::cout << "listen failed" << std::endl;
            // error(1, errno, "listen failed");
        }

        socklen_t clilen = sizeof(clientaddr);
        std::cout << "open log socket service" << std::endl;
        // 接收客户端连接
        if ((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen)) < 0)
        {
            std::cout << "accept failed" << std::endl;
            return;
        }
        is_connect = true; // 已经有客户端连接
        std::cout << "已经有客户端连接" << std::endl;
        SetThis(this);
    }

    void SocketLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (!is_connect)
        {
            std::cout << "no client connect!\n";
        }
        std::cout << "enter log sock func" << std::endl;
        if (level >= m_level)
        {
            // MutexType::Lock lock(m_mutex);
            // 按照指定格式进行格式化
            // 清空缓冲区
            send_line.str("");
            // 将日志消息读入stringstream
            send_line << m_formatter->format(logger, level, event);
            int nbytes = send_line.str().length();
            // 发送消息
            if (write(connfd, send_line.str().c_str(), nbytes) != nbytes)
            {
                std::cout << "usage: unixstreamserver <local_path>" << std::endl;
                return;
            }
        }
    }

    std::string SocketLogAppender::toYamlString()
    {
        std::cout<<"zje"<<std::endl;
        return "";
    }

}