#include <stdio.h>
#include <iostream>
#include "log.h"
#include "util.h"

int main()
{
    // 定义一个日志器
    sylar::Logger::ptr logger(new sylar::Logger("logger_zje", sylar::LogLevel::DEBUG));

    // 加入控制台输出appender
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));

    // 加入文件输出appender
    // sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./log.txt"));
    // sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    // file_appender->setFormatter(fmt);
    // file_appender->setLevel(sylar::LogLevel::ERROR);
    // logger->addAppender(file_appender);

    /**
     * LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time, const std::string &thread_name)
        : m_logger(logger), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name)
    {
    }
    */
    // 默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
    // 定义一个event
    sylar::LogEvent::ptr event(new sylar::LogEvent(logger, sylar::LogLevel::Level::DEBUG, __FILE__, __LINE__, 120, 11, 222, time(0), "zje_logger"));

    // 输出无消息
    logger->log(sylar::LogLevel::DEBUG, event);

    // 输出有消息
    event->getSS() << "not define"; // 将消息写入m_ss对象中
    logger->log(sylar::LogLevel::FATAL, event);

    // file_appender->log(logger, file_appender->getLevel(), event);

    SYLAR_LOG_INFO(logger) << "test info"; // 这样每次都会创建一个LogEvent对象
    SYLAR_LOG_ERROR(logger) << "test error";
    SYLAR_LOG_FATAL(logger) << "test fatal";

    // SYLAR_LOG_FMT_ERROR(logger, "test macro fmt error %s", "aa");
    // auto l = sylar::LoggerMgr::GetInstance()->getLogger("xx");
    // SYLAR_LOG_INFO(l) << "xxx";

    return 0;
}