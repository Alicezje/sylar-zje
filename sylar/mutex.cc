#include <stdexcept>
#include "mutex.h"

namespace sylar
{
    Semaphore::Semaphore(uint32_t count)
    {
        // 第二个参数,0:线程同步,1:进程同步
        if (sem_init(&m_semaphore, 0, count))
        {
            throw std::logic_error("sem init error");
        }
    }

    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    void Semaphore::wait()
    {
        // 如果没拿到，阻塞在这里
        if (sem_wait(&m_semaphore))
        {
            throw std::logic_error("sem_wait error");
        }
    }

    void Semaphore::notify()
    {
        if (sem_post(&m_semaphore))
        {
            throw std::logic_error("sem_post error");
        }
    }

}