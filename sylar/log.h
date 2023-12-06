#ifndef __SYLAR_LOG_H__
#define __SYLAR_LOG_H__

#include <iostream>
#include <string.h>
#include <stdint.h>
#include <memory>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include "singleton.h"
#include "util.h"
#include "thread.h"

/**
 * 使用流式方式将日志级别level的日志写入到logger
 */
#define SYLAR_LOG_LEVEL(logger, level)                                                                                   \
    if (logger->getLevel() <= level)                                                                                     \
    sylar::LogEventWrap(sylar::LogEvent::ptr(new sylar::LogEvent(logger, level,                                          \
                                                                 __FILE__, __LINE__, 0, sylar::GetThreadId(),            \
                                                                 sylar::GetFiberId(), time(0), sylar::GetThreadName()))) \
        .getSS()

/**
 * @brief 使用流式方式将日志级别debug的日志写入到logger
 */
#define SYLAR_LOG_DEBUG(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::DEBUG)

/**
 * @brief 使用流式方式将日志级别info的日志写入到logger
 */
#define SYLAR_LOG_INFO(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::INFO)

/**
 * @brief 使用流式方式将日志级别warn的日志写入到logger
 */
#define SYLAR_LOG_WARN(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::WARN)

/**
 * @brief 使用流式方式将日志级别error的日志写入到logger
 */
#define SYLAR_LOG_ERROR(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::ERROR)

/**
 * @brief 使用流式方式将日志级别fatal的日志写入到logger
 */
#define SYLAR_LOG_FATAL(logger) SYLAR_LOG_LEVEL(logger, sylar::LogLevel::FATAL)

// 获取root主日志器
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

// 更加名字获取日志
#define SYLAR_LOG_NAME(name) sylar::LoggerMgr::GetInstance()->getLogger(name)

namespace sylar
{
    // 前置声明
    class LogFormatter;
    class Logger;

    // 日志级别
    class LogLevel
    {
    public:
        enum Level
        {
            UNKNOW = 0,
            DEBUG = 1,
            INFO = 2,
            WARN = 3,
            ERROR = 4,
            FATAL = 5,
        };
        /**
         * 将日志级别转换为文本输出
         */
        static const char *ToString(LogLevel::Level level);

        /**
         * 将文本转换成日志级别
         */
        static LogLevel::Level FromString(const std::string &str);
    };

    /**
     * 日志事件
     */
    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent();
        // LogEvent(const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time);
        LogEvent(std::shared_ptr<Logger> logger,
                 LogLevel::Level level,
                 const char *file,
                 int32_t line,
                 uint32_t elapse,
                 uint32_t thread_id,
                 uint32_t fiber_id,
                 uint64_t time,
                 const std::string &thread_name);

        /**
         * 返回文件名
         */
        const char *getFile() const { return m_file; }
        /**
         * 返回行号
         */
        int32_t getLine() const { return m_line; }
        /**
         * 返回耗时
         */
        uint32_t getElapse() const { return m_elapse; }
        /**
         * 返回线程ID
         */
        uint32_t getThreadId() const { return m_threadId; }

        /**
         * 返回线程名称
         */
        std::string getThreadName() const { return m_threadName; }

        /**
         * 返回协程ID
         */
        uint32_t getFiberId() const { return m_fiberId; }
        /**
         * 返回时间
         */
        uint64_t getTime() const { return m_time; }
        /**
         * 返回日志内容
         */
        std::string getContent() const { return m_ss.str(); }

        std::stringstream &getSS() { return m_ss; }

        /**
         * 返回日志器
         */
        std::shared_ptr<Logger> getLogger() const
        {
            return m_logger;
        }
        /**
         * 返回日志级别
         */
        LogLevel::Level getLevel() const
        {
            return m_level;
        }

        /**
         * 格式化写入日志内容
         */
        void format(const char *fmt, ...);

        /**
         * 格式化写入日志内容
         */
        void format(const char *fmt, va_list al);

    private:
        const char *m_file = nullptr;     // 文件名
        int32_t m_line = 0;               // 行号
        uint32_t m_elapse = 0;            // 程序启动到现在的毫秒数
        uint32_t m_threadId = 0;          // 线程ID
        uint32_t m_fiberId = 0;           // 协程ID
        uint64_t m_time = 0;              // 时间戳
        std::string m_threadName;         // 线程名称
        std::stringstream m_ss;           // 日志内容流
        std::shared_ptr<Logger> m_logger; // 日志器
        LogLevel::Level m_level;          // 日志等级
    };

    /**
     * 日志时间包装器
     */
    class LogEventWrap
    {
    public:
        /**
         * 构造函数
         */
        LogEventWrap(LogEvent::ptr e);

        ~LogEventWrap();

        /**
         * 获取日志事件
         */
        LogEvent::ptr getEvent() const
        {
            return m_event;
        }

        /**
         * 获取日志内容流
         */
        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };

    // 日志格式器
    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        /**
         *  构造函数
         *  pattern 格式模板
         *  %m 消息
         *  %p 日志级别
         *  %r 累计毫秒数
         *  %c 日志名称
         *  %t 线程id
         *  %n 换行
         *  %d 时间
         *  %f 文件名
         *  %l 行号
         *  %T 制表符
         *  %F 协程id
         *  %N 线程名称
         *
         *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
         */
        LogFormatter(const std::string &pattern);

        /**
         * 返回格式化日志文本
         */
        std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

        /**
         * 返回输入输出流
         */
        std::ostream &format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

    public:
        /**
         * 日志内容项格式化
         */
        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;

            virtual ~FormatItem() {}
            /**
             * 格式化日志到流
             */
            virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0; // 对每个单项进行解析
        };

        /**
         * 初始化,解析日志模板
         */
        void init();

        /**
         * 是否有错误
         */
        bool isError() const
        {
            return m_error;
        }

        /**
         * @brief 返回日志模板
         */
        const std::string getPattern() const { return m_pattern; }

    private:
        std::string m_pattern;                // 根据pattern的格式来解析出信息
        std::vector<FormatItem::ptr> m_items; // 需要解析的单项
        bool m_error = false;                 // 是否有错误
    };

    /**
     * 日志输出地
     */
    class LogAppender
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;
        // 定义锁地类型，此处为自旋锁
        typedef Spinlock MutexType;

        /**
         * 虚析构函数
         */
        virtual ~LogAppender();

        /**
         * 写入日志
         * */
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

        /**
         * 将日志输出目标的配置转为YAML String
         */
        virtual std::string toYamlString() = 0;

        void setFormatter(LogFormatter::ptr val);

        LogFormatter::ptr getFormatter();

        /**
         * 获取日志级别
         */
        LogLevel::Level getLevel() const { return m_level; }

        /**
         * 设置日志级别
         */
        void setLevel(LogLevel::Level val) { m_level = val; }

    protected:
        LogLevel::Level m_level = LogLevel::DEBUG;
        // 是否有自己的日志格式器
        bool m_hasFormatter = false;
        // 写比较多，读比较少
        MutexType m_mutex;
        LogFormatter::ptr m_formatter;
    };

    // 日志器
    class Logger : public std::enable_shared_from_this<Logger>
    {
        // 方便访问Logger类的私有成员
        friend class LoggerManager;

    public:
        typedef std::shared_ptr<Logger> ptr;
        typedef Spinlock MutexType;

        /**
         * 构造函数
         */
        Logger(const std::string &name = "root");

        Logger(const std::string &name, LogLevel::Level level) : m_name(name), m_level(level)
        {
            /**
             *  构造函数
             *  pattern 格式模板
             *  %m 消息
             *  %p 日志级别
             *  %r 累计毫秒数
             *  %c 日志名称
             *  %t 线程id
             *  %n 换行
             *  %d 时间
             *  %f 文件名
             *  %l 行号
             *  %T 制表符
             *  %F 协程id
             *  %N 线程名称
             *
             *  默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
             */
            m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
        }

        /**
         * 写日志
         */
        void log(LogLevel::Level level, LogEvent::ptr event);

        /**
         * 写debug级别日志
         */
        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);
        void fatal(LogEvent::ptr event);

        /**
         * 添加日志目标
         */
        void addAppender(LogAppender::ptr appender);

        /**
         * 删除日志目标
         */
        void delAppender(LogAppender::ptr appender);

        /**
         * 清空日志目标
         */
        void clearAppenders();

        // 获取日志名称
        const std::string getName() const
        {
            return m_name;
        }

        // 获取日志级别
        LogLevel::Level getLevel() const
        {
            return m_level;
        }

        // 设置日志级别
        void setLevel(LogLevel::Level val)
        {
            m_level = val;
        }

        /**
         * 设置日志格式器
         */
        void setFormatter(LogFormatter::ptr val);

        /**
         * 设置日志格式模板
         */
        void setFormatter(const std::string &val);

        /**
         * 获取日志格式器
         */
        LogFormatter::ptr getFormatter();

        /**
         * 将日志器的配置转成YAML String
         * 包括m_name,m_level,m_appenders,m_formatter
         */
        std::string toYamlString();

    private:
        std::string m_name;                        // 名称
        LogLevel::Level m_level;                   // 日志级别
        std::vector<LogAppender::ptr> m_appenders; // Appender集合
        LogFormatter::ptr m_formatter;             // 日志格式器
        MutexType m_mutex;                         // Mutex
        Logger::ptr m_root;                        // 主日志器 如果该日志器的appender为空，则将日志输出到主日志器中
    };

    // 输出到控制台
    class StdoutLogAppender : public LogAppender
    {
    public:
        StdoutLogAppender()
        {
        }
        typedef std::shared_ptr<StdoutLogAppender> ptr;

        // 加入override关键字重写虚函数
        virtual void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;

        ~StdoutLogAppender()
        {
        }
    };

    // 输出到文件
    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

        std::string toYamlString() override;

        // 重新打开文件，文件打开成功返回True
        bool reopen();

        ~FileLogAppender()
        {
        }

    private:
        std::string m_filename;     // 文件路径
        std::ofstream m_filestream; // 文件流
        uint64_t m_lastTime = 0;    // 上次重新打开时间
    };

    
    /**
     * 日志管理类
     */
    class LoggerManager
    {
    public:
        typedef Spinlock MutexType;

        /**
         * 构造函数
         */
        LoggerManager();

        /**
         * 获取日志器
         */
        Logger::ptr getLogger(const std::string &name);

        /**
         * 初始化
         */
        void init();

        /**
         * 返回主日志器
         */
        Logger::ptr getRoot() const
        {
            return m_root;
        }

        /**
         * 将所有的日志器配置转成YAML String
         * 这个
         */
        std::string toYamlString();

    private:
        MutexType m_mutex;                            // Mutex
        std::map<std::string, Logger::ptr> m_loggers; // 日志器容器,根据名称获取日志类
        Logger::ptr m_root;                           // 主日志器
    };

    // 日志器管理类单例模式
    typedef sylar::Singleton<LoggerManager> LoggerMgr;

} // namespace sylar

#endif