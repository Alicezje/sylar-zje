#include "thread.h"
#include "log.h"
#include <iostream>
#include <unistd.h>

void fun1()
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "name: " << sylar::Thread::GetName()
                                     << " this.name " << sylar::Thread::GetThis()->getName()
                                     << " id: " << sylar::GetThreadId()
                                     << " this.id " << sylar::Thread::GetThis()->getId();
}

void fun2()
{
}

void *myThreadID1(void *)
{
    std::cout << "thread ID1" << std::endl;
    return NULL;
}

void *myThreadID2(void *)
{
    std::cout << "thread ID2" << std::endl;
    return NULL;
}

int p_thread_test()
{
    int i = 0, ret = 0;
    // 声明线程ID
    pthread_t id1, id2;

    /**
     * pthread_create
     * param 1: 绑定特定函数的线程ID
     * param 2: 线程属性指针
     * param 3: 线程运行的函数指针
     * param 4: 传递给调用函数的参数
     */

    // 创建并执行线程ID1
    ret = pthread_create(&id1, NULL, myThreadID1, NULL);
    if (ret == -1)
    {
        std::cout << "Create pthread error!" << std::endl;
        return 1;
    }

    ret = pthread_create(&id2, NULL, myThreadID2, NULL);
    if (ret == -1)
    {
        std::cout << "Create pthread error!" << std::endl;
        return 1;
    }

    pthread_join(id1, NULL); // 阻塞主线程，直到id1结束才解除
    pthread_join(id2, NULL); // 阻塞主线程，直到id2结束才解除

    std::cout << "all thread done!" << std::endl;
    return 0;
}

#define MAX 50
int number;

// 创建一把互斥锁，全局变量
// 多个线程共享
pthread_mutex_t mutex;

void *funcA_num(void *arg)
{
    for (int i = 0; i < MAX; i++)
    {
        // 如果线程A加锁成功，不阻塞
        // 如果线程B加锁成功，线程A阻塞
        pthread_mutex_lock(&mutex);
        int cur = number;
        cur++;
        usleep(10); // 将进程挂起一段时间，单位是微秒
        number = cur;
        pthread_mutex_unlock(&mutex);
        std::cout << pthread_self() << " " << number << std::endl;
    }
    return NULL;
}

void *funcB_num(void *arg)
{
    for (int i = 0; i < MAX; i++)
    {
        // a加锁成功, b线程访问这把锁的时候是锁定的
        // 线程B先阻塞, a线程解锁之后阻塞解除
        // 线程B加锁成功了
        pthread_mutex_lock(&mutex);
        int cur = number;
        cur++;
        number = cur;
        std::cout << pthread_self() << " " << number << std::endl;
        usleep(5); // 将进程挂起一段时间，单位是微秒
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void p_thread_sync_test_1()
{
    pthread_t p1, p2;

    // 初始化互斥锁
    pthread_mutex_init(&mutex, NULL);

    // 创建进程，创建成功返回0，创建失败范围错误号
    // 创建线程成功后该线程就开始执行了
    pthread_create(&p1, NULL, funcA_num, NULL);
    pthread_create(&p2, NULL, funcB_num, NULL);

    // sleep(5);

    /**
     * 这里调用pthread_join(p1, NULL);主程序并不是一直等着该线程运行完
     * 而是继续向下执行p2线程
     *
     *
     */
    pthread_join(p1, NULL); // 回收p1线程，阻塞函数，主要用来获取子线程的数据
    std::cout << "p1 finished" << std::endl;
    pthread_join(p2, NULL);
    std::cout << "p2 finished" << std::endl;

    std::cout << "finished" << std::endl;

    // 销毁互斥锁
    pthread_mutex_destroy(&mutex);
}

pthread_rwlock_t rwlock;

// 写线程处理函数
void *writeNum(void *arg)
{
    while (1)
    {
        pthread_rwlock_wrlock(&rwlock);
        int cur = number;
        cur++;
        number = cur;
        std::cout << pthread_self() << " write " << number << std::endl;
        pthread_rwlock_unlock(&rwlock);

        sleep(3);
    }
    return NULL;
}

// 读进程处理函数
void *readNum(void *arg)
{
    while (1)
    {
        pthread_rwlock_rdlock(&rwlock);
        std::cout << pthread_self() << " read " << number << std::endl;
        pthread_rwlock_unlock(&rwlock);
        sleep(2);
    }
    return NULL;
}

void p_thread_rwlock_test()
{
    // 初始化读写锁
    pthread_rwlock_init(&rwlock, NULL);

    pthread_t wtid[3];
    pthread_t rtid[5];
    for (int i = 0; i < 3; i++)
    {
        pthread_create(&wtid[i], NULL, writeNum, NULL);
    }
    for (int i = 0; i < 5; i++)
    {
        pthread_create(&rtid[i], NULL, readNum, NULL);
    }

    // 释放资源
    for (int i = 0; i < 3; ++i)
    {
        pthread_join(wtid[i], NULL);
    }

    for (int i = 0; i < 5; ++i)
    {
        pthread_join(rtid[i], NULL);
    }

    // 销毁读写锁
    pthread_rwlock_destroy(&rwlock);
}

int main()
{
    // sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();
    // SYLAR_LOG_INFO(g_logger) << "thread test begin";
    // std::vector<sylar::Thread::ptr> thrs;
    // for (int i = 0; i < 5; i++)
    // {
    //     sylar::Thread::ptr thr(new sylar::Thread(&fun1, "name_" + std::to_string(i)));
    //     thrs.push_back(thr);
    // }
    // sleep(20);
    // for (int i = 0; i < 5; i++)
    // {
    //     std::cout<<thrs[i]->GetName()<<std::endl;
    //     thrs[i]->join();
    // }
    // SYLAR_LOG_INFO(g_logger) << "thread test end";

    // p_thread_test();

    // p_thread_sync_test_1();

    // p_thread_rwlock_test();

    return 0;
}