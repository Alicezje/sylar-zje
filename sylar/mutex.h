#ifndef __SYLAR_MUTEX_H__
#define __SYLAR_MUTEX_H__

#include <semaphore.h>
#include <pthread.h>
#include <stdint.h>

#include "noncopyable.h"

namespace sylar
{
    class Semaphore : Noncopyable
    {
    public:
        /**
         * 初始化信号量,信号量个数为count
         */
        Semaphore(uint32_t count = 0);
        ~Semaphore();

        /**
         * 获取信号量
         */
        void wait();

        /**
         * 释放信号量
         */
        void notify();

    private:
        sem_t m_semaphore;
    };

    /**
     * 类似于与一个适配器
     * 写一个通用的模板类去管理他们
     * 类的构造函数加锁
     * 析构函数解锁
     */
    template <class T>
    struct ScopedLockImpl
    {
    public:
        ScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }

        ~ScopedLockImpl()
        {
            // 调用析构函数时进行解锁
            unlock();
        }

        void lock()
        {
            // 判断是否已经加锁，防止重复加锁造成死锁
            if (!m_locked)
            {
                m_mutex.lock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;
        bool m_locked; // 锁的状态
    };

    /**
     * 互斥量
     */
    class Mutex : Noncopyable
    {
    public:
        // 局部锁
        typedef ScopedLockImpl<Mutex> Lock;

        Mutex()
        {
            pthread_mutex_init(&m_mutex, nullptr);
        }

        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        void lock()
        {
            pthread_mutex_lock(&m_mutex);
        }

        void unlock()
        {
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
    };

    /**
     * 局部读锁模板实现
     */
    template <class T>
    class ReadScopedLockImpl
    {
    public:
        ReadScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }

        ~ReadScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            // 判断是否已经加锁，防止重复加锁造成死锁
            if (!m_locked)
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;    // mutex
        bool m_locked; // 锁的状态
    };

    /**
     * 局部写锁模板实现
     */
    template <class T>
    struct WriteScopedLockImpl
    {
    public:
        WriteScopedLockImpl(T &mutex) : m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }

        ~WriteScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            // 判断是否已经加锁，防止重复加锁造成死锁
            if (!m_locked)
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }

        void unlock()
        {
            if (m_locked)
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }

    private:
        T &m_mutex;    // mutex
        bool m_locked; // 锁的状态
    };

    class RWMutex : Noncopyable
    {
    public:
        // 局部读锁
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        // 局部写锁
        typedef WriteScopedLockImpl<RWMutex> WriteLock;

        RWMutex()
        {
            pthread_rwlock_init(&m_lock, nullptr);
        }

        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);
        }

        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }

        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }

        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }

    private:
        pthread_rwlock_t m_lock;
    };

    /**
     * 自旋锁
     */
    class Spinlock : Noncopyable
    {
    public:
        // 局部锁
        typedef ScopedLockImpl<Spinlock> Lock;
        /**
         * @brief 构造函数
         */
        Spinlock()
        {
            pthread_spin_init(&m_mutex, 0);
        }

        /**
         * @brief 析构函数
         */
        ~Spinlock()
        {
            pthread_spin_destroy(&m_mutex);
        }

        /**
         * @brief 上锁
         */
        void lock()
        {
            pthread_spin_lock(&m_mutex);
        }

        /**
         * @brief 解锁
         */
        void unlock()
        {
            pthread_spin_unlock(&m_mutex);
        }

    private:
        /// 自旋锁
        pthread_spinlock_t m_mutex;
    };

}

#endif