#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <queue>

/**
 * 生产者和消费者问题
 * 使用条件变量实现生产者和消费者模型，生产者有5个，消费者也有5个。
 */

#define MAX_SIZE 10

pthread_cond_t notempty; // 缓冲区非空的条件变量,只有非空时,消费者才能消费
pthread_cond_t notfull;  // 缓冲区非满的条件变量,只有非满时,生产者才能生产
pthread_mutex_t mutex;   // 互斥锁实现的buf的互斥访问

// 缓冲区
std::queue<int> buf;

/**
 * 顺序为 wait(mutex) --> wait(notfull)
 * 如何先申请mutex,再申请notfull,如果一直申请不到notfull,线程将一直等待,而此时生产者已经拿到了mutex
 * 则此时消费者会一直拿不到mutex，进不去缓存区，则就无法消费，无法消费就导致缓冲区一直为空，变为死锁
 * 但是我在实际改成wait(notfull) --> wait(mutex)，反而出现了各个线程忙等的状态，一直等
 * 实际写代码，该顺序是可行的，因为pthread_cond_wait方法会传入mutex锁，有以下操作
 *      如果已经对互斥锁mutex上锁了，则解锁，避免死锁
 *      当线程解除阻塞时，函数内部会帮助该线程再次加mutex锁，保证后续代码互斥访问
 *
 * 顺序为 wait(notfull) --> wait(mutex)
 * 理论分析应该是这个顺序对，但是实际代码运行，这个是错误，原因在于pthread_cond_wait函数
 * 会导致重复加锁，造成死锁
 */

// 生产者进程
void *producer(void *arg)
{
    while (1)
    {
        // 当缓冲区不满时，向缓冲区内生产
        pthread_mutex_lock(&mutex);
        while (buf.size() == MAX_SIZE)
        {
            // 阻塞，一直申请notfull
            pthread_cond_wait(&notfull, &mutex);
        }

        // pthread_mutex_lock(&mutex);

        buf.push(10);
        std::cout << pthread_self() << " producer product  cur size " << buf.size() << std::endl;
        pthread_mutex_unlock(&mutex);

        // 缓冲区有东西了,标记缓冲区为非空nonempty,这样消费者就可以申请到该信号量进行消费
        pthread_cond_signal(&notempty);

        sleep(2);
    }
    return NULL;
}

// 消费者进程
void *consumer(void *arg)
{
    while (1)
    {
        // 当缓冲区不空时，进行消费
        pthread_mutex_lock(&mutex); // 加锁，对缓冲区的访问是互斥的
        while (buf.empty())
        {
            // 缓冲区一直处于空的状态，需要一直申请notempty这个信号量
            pthread_cond_wait(&notempty, &mutex);
        }

        // pthread_mutex_lock(&mutex);

        // 拿走一个产品
        int res = buf.front();
        buf.pop();
        std::cout << pthread_self() << " consumer get is " << res << "cur size " << buf.size() << std::endl;

        pthread_mutex_unlock(&mutex);

        // 消费了一个产品，则缓冲区不满，释放信号量notfull
        pthread_cond_signal(&notfull);
        sleep(2);
    }
    return NULL;
}

int main()
{
    buf.push(1);
    buf.push(2);
    buf.push(3);

    pthread_t consumers[5];
    pthread_t producers[5];

    // 条件变量初始化
    pthread_cond_init(&notempty, NULL);
    pthread_cond_init(&notfull, NULL);
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < 5; i++)
    {
        // 创建生产者线程
        pthread_create(&producers[i], NULL, producer, NULL);
    }

    for (int i = 0; i < 5; i++)
    {
        // 创建消费者线程
        pthread_create(&consumers[i], NULL, consumer, NULL);
    }

    // 回收线程
    for (int i = 0; i < 5; i++)
    {
        pthread_join(producers[i], NULL);
    }
    for (int i = 0; i < 5; i++)
    {
        pthread_join(consumers[i], NULL);
    }

    // 条件变量销毁
    pthread_cond_destroy(&notempty);
    pthread_cond_destroy(&notfull);
    pthread_mutex_destroy(&mutex);

    return 0;
}