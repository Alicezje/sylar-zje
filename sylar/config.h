#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <functional>
#include "log.h"

namespace sylar
{
    /**
     * 存放一些公用的属性
     */
    class ConfigVarBase
    {
    public:
        typedef std::shared_ptr<ConfigVarBase> ptr;

        ConfigVarBase(const std::string &name, const std::string &description)
            : m_name(name),
              m_description(description)
        {
            // 转为小写
            std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
        }

        virtual ~ConfigVarBase()
        {
        }

        const std::string &getName() const
        {
            return m_name;
        }

        const std::string &getDescription() const
        {
            return m_description;
        }

        /**
         * 转为字符串
         */
        virtual std::string toString() = 0;

        /**
         * 从字符串中加载
         */
        virtual bool fromString(const std::string &val) = 0;

        virtual std::string getTypeName() const = 0;

    protected:
        std::string m_name;        // 名称
        std::string m_description; // 描述
    };

    /**
     * 数据类型转换
     * F from_type
     * T to_type
     */
    template <class F, class T>
    class LexicalCast
    {
    public:
        // 重载操作符，将v转为T类型
        T operator()(const F &v)
        {
            return boost::lexical_cast<T>(v);
        }
    };

    // ***************vector start****************

    /**
     * 数据类型转换-vector偏特化版本
     * string -> vector<T>
     */
    template <class T>
    class LexicalCast<std::string, std::vector<T>>
    {
    public:
        std::vector<T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::vector<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str(""); // 字符流置为空
                ss << node[i];
                // node[i]为一个基本类型string，需要将string转为T类型
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-vector偏特化版本
     * vector<T> -> string
     */
    template <class T>
    class LexicalCast<std::vector<T>, std::string>
    {
    public:
        std::string operator()(const std::vector<T> &v)
        {
            YAML::Node node;
            // 将vector中的每一个元素变为yaml结点
            for (auto &i : v)
            {
                // vector[i] 可能不是单个Scaler，所以需要进行YAML::Load
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************vector end****************

    // ***************list start****************

    /**
     * 数据类型转换-list偏特化版本
     * string -> list<T>
     */
    template <class T>
    class LexicalCast<std::string, std::list<T>>
    {
    public:
        std::list<T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::list<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str(""); // 字符流置为空
                ss << node[i];
                // node[i]为一个基本类型string，需要将string转为T类型
                vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-list偏特化版本
     * list<T> -> string
     */
    template <class T>
    class LexicalCast<std::list<T>, std::string>
    {
    public:
        std::string operator()(const std::list<T> &v)
        {
            YAML::Node node;
            // 将list中的每一个元素变为yaml结点
            for (auto &i : v)
            {
                // list[i] 可能不是单个Scaler，所以需要进行YAML::Load
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************list end****************

    // ***************set start****************

    /**
     * 数据类型转换-set偏特化版本
     * string -> set<T>
     */
    template <class T>
    class LexicalCast<std::string, std::set<T>>
    {
    public:
        std::set<T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str(""); // 字符流置为空
                ss << node[i];
                // node[i]为一个基本类型string，需要将string转为T类型
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-set偏特化版本
     * set<T> -> string
     */
    template <class T>
    class LexicalCast<std::set<T>, std::string>
    {
    public:
        std::string operator()(const std::set<T> &v)
        {
            YAML::Node node;
            // 将set中的每一个元素变为yaml结点
            for (auto &i : v)
            {
                // set[i] 可能不是单个Scaler，所以需要进行YAML::Load
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************set end****************

    // ***************unordered_set start****************

    /**
     * 数据类型转换-unordered_set偏特化版本
     * string -> unordered_set<T>
     */
    template <class T>
    class LexicalCast<std::string, std::unordered_set<T>>
    {
    public:
        std::unordered_set<T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::unordered_set<T> vec;
            std::stringstream ss;
            for (size_t i = 0; i < node.size(); i++)
            {
                ss.str(""); // 字符流置为空
                ss << node[i];
                // node[i]为一个基本类型string，需要将string转为T类型
                vec.insert(LexicalCast<std::string, T>()(ss.str()));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-unordered_set偏特化版本
     * unordered_set<T> -> string
     */
    template <class T>
    class LexicalCast<std::unordered_set<T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_set<T> &v)
        {
            YAML::Node node;
            // 将set中的每一个元素变为yaml结点
            for (auto &i : v)
            {
                // set[i] 可能不是单个Scaler，所以需要进行YAML::Load
                node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************unordered_set end****************

    // ***************map start****************

    /**
     * 数据类型转换-map偏特化版本
     * string -> map<std::string,T>
     */
    template <class T>
    class LexicalCast<std::string, std::map<std::string, T>>
    {
    public:
        std::map<std::string, T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str(""); // 字符流置为空
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-map偏特化版本
     * map<std::string,T> -> string
     */
    template <class T>
    class LexicalCast<std::map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::map<std::string, T> &v)
        {
            YAML::Node node;
            // 将set中的每一个元素变为yaml结点
            for (auto it = v.begin(); it != v.end(); it++)
            {
                node[it->first] = YAML::Load(LexicalCast<T, std::string>()(it->second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************map end****************

    // ***************unordered_map start****************

    /**
     * 数据类型转换-unordered_map偏特化版本
     * string -> unordered_map<std::string,T>
     */
    template <class T>
    class LexicalCast<std::string, std::unordered_map<std::string, T>>
    {
    public:
        std::unordered_map<std::string, T> operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            typename std::unordered_map<std::string, T> vec;
            std::stringstream ss;
            for (auto it = node.begin(); it != node.end(); it++)
            {
                ss.str(""); // 字符流置为空
                ss << it->second;
                vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
            }
            return vec;
        }
    };

    /**
     * 数据类型转换-unordered_map偏特化版本
     * unordered_map<std::string,T> -> string
     */
    template <class T>
    class LexicalCast<std::unordered_map<std::string, T>, std::string>
    {
    public:
        std::string operator()(const std::unordered_map<std::string, T> &v)
        {
            YAML::Node node;
            // 将set中的每一个元素变为yaml结点
            for (auto it = v.begin(); it != v.end(); it++)
            {
                node[it->first] = YAML::Load(LexicalCast<T, std::string>()(it->second));
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************unordered_map end****************

    // 重写operator()方法
    // FromStr T& operator()(const std::string&)
    // ToStr std::string operator()(const T&)
    // 特例化模板
    template <class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
    class ConfigVar : public ConfigVarBase
    {
    public:
        // 读写锁
        typedef RWMutex RWMutexType;
        typedef std::shared_ptr<ConfigVar> ptr;
        typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;

        ConfigVar(const std::string &name, const T &val, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(val)
        {
        }

        /**
         * 转为字符串
         */
        std::string toString() override
        {
            try
            {
                RWMutexType::ReadLock lock(m_mutex);
                return ToStr()(m_val);
                // return boost::lexical_cast<std::string>(m_val);
            }
            catch (std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception"
                                                  << e.what() << " convert: " << typeid(m_val).name() << " to string";
            }
            return "";
        }

        /**
         * 从字符串中加载
         */
        bool fromString(const std::string &val) override
        {
            try
            {
                setValue(FromStr()(val));
                // m_val = boost::lexical_cast<T>(val);
            }
            catch (std::exception &e)
            {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << " ConfigVar::fromString "
                                                  << e.what() << " convert: string to " << typeid(m_val).name();
            }
            return false;
        }

        const T getValue()
        {
            RWMutexType::ReadLock lock(m_mutex);
            return m_val;
        }

        /**
         * 设置值的时候，监听值是否发生，如果发生变化，做相应的操作
         */
        void setValue(const T &val)
        {
            {
                RWMutexType::ReadLock lock(m_mutex);
                // 这里有比较运算，需要在自定义类中重载 ==
                if (val == m_val)
                {
                    return;
                }
                for (auto &i : m_cbs)
                {
                    // 执行回调函数，回调函数可以有多个，挨个调用
                    i.second(m_val, val);
                }
            }
            RWMutexType::WriteLock lock(m_mutex);
            // 赋值
            m_val = val;
        }

        std::string getTypeName() const override
        {
            return typeid(T).name();
        }

        /**
         * 添加变化回调函数
         * 返回该回调函数对应的唯一id,用于删除回调
         */
        uint64_t addListener(on_change_cb cb)
        {
            static uint64_t s_fun_id = 0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_fun_id;
            m_cbs[s_fun_id] = cb;
            return s_fun_id;
        }

        // 删除监听
        void delListener(uint64_t key)
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);
        }

        // 获得监听器
        on_change_cb getListener(uint64_t key)
        {
            RWMutexType::ReadLock lock(m_mutex);
            auto it = m_cbs.find(key);
            return it == m_cbs.end() ? nullptr : it->second;
        }

        // 清空监听器
        void clearListener()
        {
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.clear();
        }

    private:
        RWMutexType m_mutex;
        T m_val; // 配置的值为value
        // typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;
        // 变更回调函数组<key,回调函数> uint64_t key,要求唯一，一般可以用hash值
        std::map<uint64_t, on_change_cb> m_cbs;
    };

    class Config
    {
    public:
        // 字符串 --> 配置变量
        typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
        // 读写锁
        typedef RWMutex RWMutexType;

        // 静态成员函数
        /**
         * 查找name,如果没有将插入
         */
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name,
                                                 const T &default_value,
                                                 const std::string &description = "")
        {
            RWMutexType::WriteLock lock(GetMutex());
            auto it = GetDatas().find(name); // 寻找是否存在key值
            if (it != GetDatas().end())
            {
                // 查看类型是否相同
                auto temp = std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
                auto tmp = Lookup<T>(name); // 调用下面写好的函数
                if (tmp)
                {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists";
                    return tmp;
                }
                else
                {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name=" << name << " exists but type not"
                                                      << typeid(T).name()
                                                      << " real type" << it->second->getTypeName()
                                                      << " " << it->second->toString();
                    return nullptr;
                }
            }

            // 如果没找到
            if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos)
            {
                // name中找不到上述字符
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid" << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v; // 放进入
            return v;
        }

        /**
         * 根据name查找ConfigVar
         */
        template <class T>
        static typename ConfigVar<T>::ptr Lookup(const std::string &name)
        {
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if (it == GetDatas().end())
            {
                return nullptr;
            }
            // 转为智能指针  it->second访问值,it->first访问键
            return std::dynamic_pointer_cast<ConfigVar<T>>(it->second);
        }
        /**
         * 从yaml中加载配置
         */
        static void LoadFromYaml(const YAML::Node &root);

        /**
         * 根据name查找对应的ConfigVarBase
         */
        static ConfigVarBase::ptr LookupBase(const std::string &name);

        /**
         * @brief 遍历配置模块里面所有配置项
         * @param[in] cb 配置项回调函数
         */
        static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

    private:
        /**
         * 配置项的所有配置项
         */
        static ConfigVarMap &GetDatas()
        {
            static ConfigVarMap s_datas;
            return s_datas;
        }
        /**
         * 放在静态方法里，静态锁
         * 为什么使用静态方法的形式返回读写锁？
         * 因为Config类创建一个全局变量，全局变量的初始化没有严格的顺序
         * 如果不使用静态方法，而使用静态成员的方式，如果静态成员的初始化顺序，比执行那个方法要晚
         * 就会出现，锁还没有构建成功，会出现内存错误
         */

        /**
         * 配置项的所有RWMutex
         */
        static RWMutexType &GetMutex()
        {
            static RWMutexType s_mutex;
            return s_mutex;
        }
    };

}

#endif