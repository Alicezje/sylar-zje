/**
 * 对象不可拷贝封装
 * 禁用拷贝构造函数
 * 禁用赋值函数
 */
#ifndef __SYLAR_NONCOPYABLE_H__
#define __SYLAR_NONCOPYABLE_H__

namespace sylar
{
    class Noncopyable
    {
    public:
        /**
         * 默认构造函数
         */
        Noncopyable() = default;

        /**
         * 默认析构函数
         */
        ~Noncopyable() = default;

        /**
         * 禁用拷贝构造函数
         */
        Noncopyable(const Noncopyable &) = delete;

        /**
         * 禁用赋值运算
         */
        Noncopyable &operator=(const Noncopyable &) = delete;
    };
}

#endif
