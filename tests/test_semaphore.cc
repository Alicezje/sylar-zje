#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <queue>

/**
 * 生产者和消费者问题
 * 使用信号量实现生产者和消费者模型，生产者有5个，消费者也有5个。
 */

#define MAX_SIZE 10

sem_t empty;           // 信号量empty,表示缓冲区中空闲位置
sem_t full;            // 信号量full,表示缓冲区中资源个数
pthread_mutex_t mutex; // 互斥锁实现的buf的互斥访问

// 缓冲区
std::queue<int> buf;

// 生产者进程
void *producer(void *arg)
{
    while (1)
    {
        // 当缓冲区不满时，向缓冲区内生产
        sem_wait(&empty);
        pthread_mutex_lock(&mutex); // 如果加锁操作放到sem_wait前面有可能会产生死锁

        buf.push(10);
        std::cout << pthread_self() << " producer product  cur size " << buf.size() << std::endl;

        pthread_mutex_unlock(&mutex);
        sem_post(&full);

        sleep(2);
    }
    return NULL;
}

// 消费者进程
void *consumer(void *arg)
{
    while (1)
    {
        sem_wait(&full);
        pthread_mutex_lock(&mutex); // 如果加锁操作放到sem_wait前面有可能会产生死锁

        // 拿走一个产品
        int res = buf.front();
        buf.pop();
        std::cout << pthread_self() << " consumer get is " << res << "cur size " << buf.size() << std::endl;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty);

        sleep(2);
    }
    return NULL;
}

int main()
{
    pthread_t consumers[5];
    pthread_t producers[5];

    // 信号量初始化
    sem_init(&empty, 0, MAX_SIZE); // 初始空闲个数为MAX_SIZE
    sem_init(&full, 0, 0);         // 初始资源个数为0

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
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}