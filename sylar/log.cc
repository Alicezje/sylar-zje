#include "log.h"
#include <iostream>
#include <map>
#include <functional>
#include <memory>
#include <time.h>
#include <string.h>
#include <stdarg.h>

namespace sylar
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;    \
        break;

            XX(DEBUG);
            XX(INFO);
            XX(WARN);
            XX(ERROR);
            XX(FATAL);
#undef XX

        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define YY(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        // 小写
        YY(DEBUG, debug);
        YY(INFO, info);
        YY(WARN, warn);
        YY(ERROR, error);
        YY(FATAL, fatal);
        // 大写
        YY(DEBUG, DEBUG);
        YY(INFO, INFO);
        YY(WARN, WARN);
        YY(ERROR, ERROR);
        YY(FATAL, FATAL);
        return LogLevel::UNKNOW;
#undef YY
    }

    LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        // LogEventWrap 析构时 将stringstream中的记录输出到日志
        m_event->getLogger()->log(m_event->getLevel(), m_event);
    }

    LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name)
        : m_logger(logger), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name)
    {
    }

    void LogEvent::format(const char *fmt, ...)
    {
        va_list al;
        va_start(al, fmt);
        format(fmt, al);
        va_end(al);
    }

    void LogEvent::format(const char *fmt, va_list al)
    {
        char *buf = nullptr;
        // 它可以通过可变参数创建一个格式化的字符串
        // 并将其存储在动态分配的内存中。
        // 将其存储在一个指向字符数组的指针中
        int len = vasprintf(&buf, fmt, al);
        if (len != -1)
        {
            // 将日志内容输出
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    std::stringstream &LogEventWrap::getSS()
    {
        return m_event->getSS();
    }

    /**
     * Logger类的方法实现
     */
    // Logger::Logger(const std::string &name,LogLevel::Level level) : m_name(name), m_level(level)
    // {
    //     m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    // }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        if (!appender->getFormatter())
        {
            appender->setFormatter(m_formatter);
        }
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void Logger::clearAppenders()
    {
        // MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

    void Logger::setFormatter(LogFormatter::ptr val)
    {
        // MutexType::Lock lock(m_mutex);
        m_formatter = val;

        for (auto &i : m_appenders)
        {
            // MutexType::Lock ll(i->m_mutex);
            if (!i->m_hasFormatter)
            {
                i->m_formatter = m_formatter;
            }
        }
    }

    void Logger::setFormatter(const std::string &val)
    {
        std::cout << "---" << val << std::endl;
        sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
        if (new_val->isError())
        {
            std::cout << "Logger setFormatter name=" << m_name
                      << " value=" << val << " invalid formatter"
                      << std::endl;
            return;
        }
        // m_formatter = new_val;
        setFormatter(new_val);
    }

    // std::string Logger::toYamlString()
    // {
    //     MutexType::Lock lock(m_mutex);
    //     YAML::Node node;
    //     node["name"] = m_name;
    //     if (m_level != LogLevel::UNKNOW)
    //     {
    //         node["level"] = LogLevel::ToString(m_level);
    //     }
    //     if (m_formatter)
    //     {
    //         node["formatter"] = m_formatter->getPattern();
    //     }

    //     for (auto &i : m_appenders)
    //     {
    //         node["appenders"].push_back(YAML::Load(i->toYamlString()));
    //     }
    //     std::stringstream ss;
    //     ss << node;
    //     return ss.str();
    // }

    LogFormatter::ptr Logger::getFormatter()
    {
        // MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    /**
     * 日志器写日志,总的日志器
     * 调用该方法会将
     */
    void Logger::log(LogLevel::Level level, LogEvent::ptr event)
    {
        // 仅输出级别>=m_level的日志
        if (level >= m_level)
        {
            auto self = shared_from_this();
            // MutexType::Lock lock(m_mutex);
            if (!m_appenders.empty())
            {
                for (auto &i : m_appenders)
                {
                    // 遍历每一个appender
                    i->log(self, level, event);
                }
            }
            else if (m_root)
            {
                // 如果没有appender
                // 这里会输出东西吗？？
                m_root->log(level, event);
            }
        }
    }

    void Logger::debug(LogEvent::ptr event)
    {
        log(LogLevel::DEBUG, event);
    }
    void Logger::info(LogEvent::ptr event)
    {
        log(LogLevel::INFO, event);
    }
    void Logger::warn(LogEvent::ptr event)
    {
        log(LogLevel::WARN, event);
    }
    void Logger::error(LogEvent::ptr event)
    {
        log(LogLevel::ERROR, event);
    }
    void Logger::fatal(LogEvent::ptr event)
    {
        log(LogLevel::FATAL, event);
    }

    void LogAppender::setFormatter(LogFormatter::ptr val)
    {
        // MutexType::Lock lock(m_mutex);
        m_formatter = val;
        if (m_formatter)
        {
            m_hasFormatter = true;
        }
        else
        {
            m_hasFormatter = false;
        }
    }

    LogFormatter::ptr LogAppender::getFormatter()
    {
        // 保证原子性
        //  MutexType::Lock lock(m_mutex);
        return m_formatter;
    }

    LogAppender::~LogAppender()
    {
    }

    std::string Logger::toYamlString()
    {
        return "";
    }

    /**
     * StdoutLogAppender类的方法实现
     */
    void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            // 按照指定格式进行格式化
            std::cout << m_formatter->format(logger, level, event);
        }
    }

    std::string StdoutLogAppender::toYamlString()
    {
        return 0;
    }

    /**
     * FileLogAppender类的方法实现
     */

    FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename)
    {
        reopen();
    }

    bool FileLogAppender::reopen()
    {
        if (m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

    void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        if (level >= m_level)
        {
            m_filestream << m_formatter->format(logger, level, event);
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        return "";
    }

    // **********************
    // 消息Format
    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    // 级别Format
    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(level);
        }
    };

    // 启动时间Format
    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    // 名称
    class NameFormatItem : public LogFormatter::FormatItem
    {
    public:
        NameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLogger()->getName();
        }
    };

    // 线程名称
    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };

    // 日志名称Format
    class LogNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        LogNameFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << logger->getName();
        }
    };

    // 线程IDFormat
    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };

    // 协程IDFormat
    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    // 日志时间Format
    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (m_format.empty())
            {
                m_format = "%Y-%m-%d %H:%M:%S";
            }
        }

        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm);
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm);
            os << buf;
        }

    private:
        std::string m_format;
    };

    // 文件名Format
    class FilenameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FilenameFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };

    // 行号Format
    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    // 新的一行Format
    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &fmt)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    // String Format
    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string str) : m_string(str)
        {
        }
        void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str = "") {}
        void format(std::ostream &os, Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override
        {
            os << "\t";
        }

    private:
        std::string m_string;
    };

    /**
     * LogFormatter类的方法实现
     */
    LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
    {
        init();
    }

    std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        std::stringstream ss;
        // 遍历每一个单项,依次进行格式化
        for (auto i : m_items)
        {
            // 传入ss为引用，其内部将os输出流保存到ss中
            i->format(ss, logger, level, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
    {
        for (auto &i : m_items)
        {
            // 遍历每一个xxxFormatItem
            i->format(ofs, logger, level, event);
        }
        return ofs;
    }

    // %xxx %xxx{xxx} %%
    void LogFormatter::init()
    {
        // 解析核心代码
        // m_pattern: %d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n
        // 三元组<str, format, type>
        // type==0  StringFormatItem    用于存放m_pattern的普通字符,如'[',']',':'
        // type==1  其他的FormatItem
        // str      m p r ...
        std::vector<std::tuple<std::string, std::string, int>> vec;
        // 存放格式化字符 Y m d etc...
        std::string nstr;
        for (size_t i = 0; i < m_pattern.size(); ++i)
        {
            if (m_pattern[i] != '%')
            {
                // 如果不是%号
                // nstr字符串后添加1个字符m_pattern[i]
                nstr.append(1, m_pattern[i]);
                continue;
            }

            if ((i + 1) < m_pattern.size())
            {
                // m_pattern[i]是% && m_pattern[i + 1] == '%' ==> 两个%,第二个%当作普通字符
                if (m_pattern[i + 1] == '%')
                {
                    nstr.append(1, '%');
                    continue;
                }
            }

            // m_pattern[i]是% && m_pattern[i + 1] != '%', 需要进行解析
            size_t n = i + 1;     // 跳过'%',从'%'的下一个字符开始解析
            int fmt_status = 0;   // 是否解析大括号内的内容: 已经遇到'{',但是还没有遇到'}' 值为1
            size_t fmt_begin = 0; // 大括号开始的位置

            std::string str;
            std::string fmt; // 存放'{}'中间截取的字符
            // 从m_pattern[i+1]开始遍历
            while (n < m_pattern.size())
            {
                // m_pattern[n]不是字母 & m_pattern[n]不是'{' & m_pattern[n]不是'}'
                if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
                {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    break;
                }
                if (fmt_status == 0)
                {
                    if (m_pattern[n] == '{')
                    {
                        // 遇到'{',将前面的字符截取
                        str = m_pattern.substr(i + 1, n - i - 1);
                        // std::cout << "*" << str << std::endl;
                        fmt_status = 1; // 标志进入'{'
                        fmt_begin = n;  // 标志进入'{'的位置
                        ++n;
                        continue;
                    }
                }
                else if (fmt_status == 1)
                {
                    if (m_pattern[n] == '}')
                    {
                        // 遇到'}',将和'{'之间的字符截存入fmt
                        fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                        // std::cout << "#" << fmt << std::endl;
                        fmt_status = 0;
                        ++n;
                        // 找完一组大括号就退出循环
                        break;
                    }
                }
                ++n;
                // 判断是否遍历结束
                if (n == m_pattern.size())
                {
                    if (str.empty())
                    {
                        str = m_pattern.substr(i + 1);
                    }
                }
            }

            if (fmt_status == 0)
            {
                if (!nstr.empty())
                {
                    // 保存其他字符 '['  ']'  ':'
                    vec.push_back(std::make_tuple(nstr, std::string(), 0));
                    nstr.clear();
                }
                // fmt:寻找到的格式
                vec.push_back(std::make_tuple(str, fmt, 1));
                // 调整i的位置继续向后遍历
                i = n - 1;
            }
            else if (fmt_status == 1)
            {
                // 没有找到与'{'相对应的'}' 所以解析报错，格式错误
                std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
                m_error = true;
                vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
            }
        }

        if (!nstr.empty())
        {
            vec.push_back(std::make_tuple(nstr, "", 0));
        }
        /**
         * s_format_items<格式标识:string,对应的对象:FormatItem>
         * 不同的FormatItem可以实现自己的format方法进行日志格式化
         * new 一个FormatItem对象,传入格式为fmt
         */
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
    }

            XX(m, MessageFormatItem),    // m:消息
            XX(p, LevelFormatItem),      // p:日志级别
            XX(r, ElapseFormatItem),     // r:累计毫秒数
            XX(c, NameFormatItem),       // c:日志名称
            XX(t, ThreadIdFormatItem),   // t:线程id
            XX(n, NewLineFormatItem),    // n:换行
            XX(d, DateTimeFormatItem),   // d:时间
            XX(f, FilenameFormatItem),   // f:文件名
            XX(l, LineFormatItem),       // l:行号
            XX(T, TabFormatItem),        // T:Tab
            XX(F, FiberIdFormatItem),    // F:协程id
            XX(N, ThreadNameFormatItem), // N:线程名称
#undef XX
        };

        for (auto &i : vec)
        {
            // 三元组<str, format, type>
            if (std::get<2>(i) == 0)
            {
                // 存放需要解析的单项,这个为普通字符
                m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
            }
            else
            {
                // 从s_format_items寻找,返回对应的对象
                auto it = s_format_items.find(std::get<0>(i)); // 获得i的第1项
                if (it == s_format_items.end())
                {
                    // 格式未从s_format_items中找到
                    m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                    m_error = true;
                }
                else
                {
                    // 将需要解析单项的xxxFormatItem存入
                    m_items.push_back(it->second(std::get<1>(i)));
                }
            }

            // std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
        }
        // std::cout << m_items.size() << std::endl;
    }

    void LoggerManager::init(){

    }

    LoggerManager::LoggerManager()
    {
        m_root.reset(new Logger("root",LogLevel::DEBUG));
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        // MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        logger->m_root = m_root;
        m_loggers[name] = logger;
        return logger;
    }
}