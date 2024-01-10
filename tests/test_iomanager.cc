#include "log.h"
#include "iomanager.h"
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

int sock = 0;

void test_fiber()
{
    SYLAR_LOG_INFO(g_logger) << "test_fiber sock=" << sock;

    // sleep(3);

    // close(sock);
    // sylar::IOManager::GetThis()->cancelAll(sock);

    // 创建socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // 设成非阻塞模式
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "127.1.1.0", &addr.sin_addr.s_addr);

    if (!connect(sock, (const sockaddr *)&addr, sizeof(addr)))
    {
        // 建立联机失败
    }
    else if (errno == EINPROGRESS)
    {
        SYLAR_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);

        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::READ, []()
                                              { SYLAR_LOG_INFO(g_logger) << "read callback"; });

        sylar::IOManager::GetThis()->addEvent(sock, sylar::IOManager::WRITE, []()
                                              {
            SYLAR_LOG_INFO(g_logger) << "write callback";
            //close(sock);
            sylar::IOManager::GetThis()->cancelEvent(sock, sylar::IOManager::READ);
            close(sock); });
    }
    else
    {
        SYLAR_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
    }
}

void test1()
{
    // std::cout << "EPOLLIN=" << EPOLLIN
    //           << " EPOLLOUT=" << EPOLLOUT << std::endl;
    sylar::IOManager iom(2, false);
    // 将test_fiber假如调度队列中
    iom.schedule(&test_fiber);
}

// sylar::Timer::ptr s_timer;
// void test_timer() {
//     sylar::IOManager iom(2);
//     s_timer = iom.addTimer(1000, [](){
//         static int i = 0;
//         SYLAR_LOG_INFO(g_logger) << "hello timer i=" << i;
//         if(++i == 3) {
//             s_timer->reset(2000, true);
//             //s_timer->cancel();
//         }
//     }, true);
// }

class A
{
public:
    static void test()
    {
        std::cout << "A class" << std::endl;
    }

    void func()
    {
        std::cout << "scheduler A func" << std::endl;
    }
};

class B : public A
{
public:
    static void test()
    {
        std::cout << "B class" << std::endl;
    }

    // void func()
    // {
    //     std::cout << "scheduler B func" << std::endl;
    // }
};

int main(int argc, char **argv)
{
    // test1();
    // test_timer();
    A a;
    // a.test();
    B b;
    // b.test();
    b.func();

    A *p = &b;
    p->test();
    p->func();
    return 0;
}
