#ifndef __LOG_CONFIG_H__
#define __LOG_CONFIG_H__

#include "log.h"
#include "config.h"
#include <iostream>
#include <string.h>

/**
 * 日志配置
 * 定义日志配置需要用到类、函数、偏特化函数、监听器等
 */

namespace sylar
{
    struct LogAppenderDefine
    {
        // 具体的yaml文件中appender定义
        int type; // 1 File,2 Stdout
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::string file;

        // 重载等于运算符，ConfigVar->setValue会用到
        bool operator==(const LogAppenderDefine &oth) const
        {
            return type == oth.type &&
                   level == oth.level &&
                   formatter == oth.formatter &&
                   file == oth.file;
        }
    };

    struct LogDefine
    {
        // 与yaml文件中定义的一致，对应起来，这类似于自定义类Person
        std::string name;
        LogLevel::Level level = LogLevel::UNKNOW;
        std::string formatter;
        std::vector<LogAppenderDefine> appenders;

        // 重载等于运算符，ConfigVar->setValue会用到
        bool operator==(const LogDefine &oth) const
        {
            return name == oth.name &&
                   level == oth.level &&
                   formatter == oth.formatter &&
                   appenders == oth.appenders;
        }

        bool operator<(const LogDefine &oth) const
        {
            return name < oth.name;
        }

        bool isValid() const
        {
            return !name.empty();
        }
    };

    /**
     * string -> LogDefine
     */
    template <>
    class LexicalCast<std::string, LogDefine>
    {
    public:
        LogDefine operator()(const std::string &v)
        {
            YAML::Node n = YAML::Load(v); // 将字符串使用yaml-cpp加载
            LogDefine ld;
            if (!n["name"].IsDefined())
            {
                // 不存在name这个key
                std::cout << "log config error: name is null, " << n
                          << std::endl;
                throw std::logic_error("log config name is null");
            }
            // 设置name
            ld.name = n["name"].as<std::string>();
            // 设置level
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            // 设置格式
            if (n["formatter"].IsDefined())
            {
                ld.formatter = n["formatter"].as<std::string>();
            }
            // 设置appender
            if (n["formatter"].IsDefined())
            {
                // 遍历YAML树中n["appenders"]的每一个节点
                for (size_t x = 0; x < n["appenders"].size(); x++)
                {
                    auto a = n["appenders"][x]; // 取出节点
                    if (!a["type"].IsDefined())
                    {
                        // appender 没有type，则出错
                        std::cout << "log config error: appender type is null, " << a
                                  << std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if (type == "FileLogAppender")
                    {
                        lad.type = 1;
                        if (!a["file"].IsDefined())
                        {
                            std::cout << "log config error: fileappender file is null, " << a
                                      << std::endl;
                            continue;
                        }
                        lad.file = a["file"].as<std::string>();
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    }
                    else if (type == "StdoutLogAppender")
                    {
                        lad.type = 2;
                        if (a["formatter"].IsDefined())
                        {
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    }
                    else
                    {
                        std::cout << "log config error: appender type is invalid, " << a
                                  << std::endl;
                        continue;
                    }
                    // 每找到一个就push_back到vector中
                    ld.appenders.push_back(lad);
                }
            }

            return ld;
        }
    };

    /**
     * LogDefine -> string
     */
    template <>
    class LexicalCast<LogDefine, std::string>
    {
    public:
        std::string operator()(const LogDefine &i)
        {
            // 将LogDefine对象中的值依次设置到YAML::Node中
            YAML::Node n;
            n["name"] = i.name;
            if (i.level != LogLevel::UNKNOW)
            {
                n["level"] = LogLevel::ToString(i.level);
            }
            if (!i.formatter.empty())
            {
                n["formatter"] = i.formatter;
            }

            for (auto &a : i.appenders)
            {
                // 遍历每个appender，需要根据类型设置
                YAML::Node na;
                if (a.type == 1)
                {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                }
                else if (a.type == 2)
                {
                    na["type"] = "StdoutLogAppender";
                }
                // 每个单独的appender也要单独设置level和formatter
                if (a.level != LogLevel::UNKNOW)
                {
                    na["level"] = LogLevel::ToString(a.level);
                }

                if (!a.formatter.empty())
                {
                    na["formatter"] = a.formatter;
                }

                n["appenders"].push_back(na);
            }
            std::stringstream ss;
            ss << n;
            return ss.str();
        }
    };

    struct LogIniter
    {
        sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines =
            sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");
        LogIniter()
        {
            // 绑定监听函数
            g_log_defines->addListener(0XF1E1231, [](const std::set<LogDefine> &old_value,
                                                     const std::set<LogDefine> &new_value)
                                       {
                                        // SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"on_logger_conf_changed";
                                           // 新增
                                           for (auto &i : new_value)
                                           {
                                               auto it = old_value.find(i);
                                               sylar::Logger::ptr logger;
                                               if (it == old_value.end())
                                               {
                                                    // 在old_value中未找个该值，则为新增节点
                                                   // 新增logger,SYLAR_LOG_NAME 如果找不到就新建一个
                                                   logger=SYLAR_LOG_NAME(i.name);
                                               }
                                               else
                                               {
                                                   if (!(i == *it))
                                                   {
                                                       // 发生改变
                                                       // 修改logger
                                                       logger=SYLAR_LOG_NAME(i.name);
                                                   }else{
                                                    // 未做什么改动
                                                    continue;
                                                   }
                                               }
                                               // 对于新增的值，要设置日志级别
                                               logger->setLevel(i.level);
                                               if(!i.formatter.empty()){
                                                logger->setFormatter(i.formatter);
                                               }
                                                // 清空appender
                                               logger->clearAppenders();

                                               for(auto& a:i.appenders){
                                                sylar::LogAppender::ptr ap;
                                                if(a.type==1){
                                                    ap.reset(new FileLogAppender(a.file));
                                                }else if(a.type==2){
                                                    ap.reset(new StdoutLogAppender);
                                                    // if(!sylar::EnvMgr::GetInstance()->has("d")) {
                                                    // ap.reset(new StdoutLogAppender);
                                                    // } else {
                                                        // continue;
                                                    // }
                                                }
                                                ap->setLevel(a.level);
                                                if(!a.formatter.empty()){
                                                    LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                                                    if(!fmt->isError()){
                                                        ap->setFormatter(fmt);
                                                    }else{
                                                        std::cout << "log.name=" << i.name << " appender type=" << a.type
                                      << " formatter=" << a.formatter << " is invalid" << std::endl;
                                                    }
                                                }
                                                logger->addAppender(ap);
                                               }
                                           }

                                           // 删除
                                           for (auto &i : old_value)
                                           {
                                                // 在new_value中找不到
                                               auto it = new_value.find(i);
                                               if (it == new_value.end())
                                               {
                                                   // 删除logger
                                                   // 可以将日志关闭，或者日志级别设定很高，这样就不会真的触发写日志了
                                                   auto logger = SYLAR_LOG_NAME(i.name);
                                                   logger->setLevel((LogLevel::Level)100);
                                                   logger->clearAppenders();
                                               }
                                           } });
        }
    };
}

#endif