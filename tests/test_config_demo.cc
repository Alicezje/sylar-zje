#include <string>
#include "config.h"
#include "log.h"
#include "util.h"
#include <yaml-cpp/yaml.h>

namespace sylar
{

    class User
    {
    public:
        User() {}

        User(std::string m_name, int m_age, std::string m_passwd)
            : name(m_name), age(m_age), passwd(m_passwd)
        {
        }

        std::string getName() const
        {
            return name;
        }

        void setName(std::string m_name)
        {
            name = m_name;
        }

        int getAge() const
        {
            return age;
        }

        void setAge(int m_age)
        {
            age = m_age;
        }

        std::string getPasswd() const
        {
            return passwd;
        }

        void setPasswd(std::string m_passwd)
        {
            passwd = m_passwd;
        }

        std::string toString() const
        {
            std::stringstream ss;
            ss << "[ " + name + " " + std::to_string(age) + " " + passwd + "" + " ]";
            return ss.str();
        }

        bool operator==(const User &oth) const
        {
            return name == oth.getName() && age == oth.getAge() && passwd == oth.getPasswd();
        }

    private:
        std::string name = "";
        int age = 0;
        std::string passwd = "";
    };

    /**
     * 存放系统的一些配置信息
     */
    class System
    {
        // 从yaml文件中加载配置，配置的信息包括用户（包含属性：用户名，年龄，用户密码）
        // 可以有多个用户，其他配置信息包括系统IP地址、系统mac地址、端口号，要求定义函数，将这些变量的值从yaml中加载。
    public:
        System()
        {
        }

        void init()
        {
        }

        /**
         * 从yaml文件中加载配置
         */
        void LoadfromYaml(std::string yaml_path)
        {
            // 向系统中初始化设置一些键
            // ip
            ConfigVar<std::string>::ptr config_ip = Config::Lookup("system.ip", std::string(), "system ip address");
            // mac
            ConfigVar<std::string>::ptr config_mac = Config::Lookup("system.mac", std::string(), "system ip mac");
            // port
            ConfigVar<int>::ptr config_port = Config::Lookup("system.port", (int)80, "system port");
            // user_vec
            ConfigVar<std::vector<User>>::ptr config_user_vec = Config::Lookup("system.user_vec", std::vector<User>{User()}, "system user_vec");

            // ConfigVar<std::vector<int>>::ptr config_int_vec = Config::Lookup("system.int_vec", std::vector<int>{1, 2, 3}, "system int_vec");
            ConfigVar<User>::ptr config_user = Config::Lookup("system.user", User(), "system user");

            // 加载yaml文件
            YAML::Node root = YAML::LoadFile(yaml_path);
            // 从yaml文件中加载配置信息，存放到了s_datas中
            sylar::Config::LoadFromYaml(root);

            // 从s_datas中获取值，放入System类中
            ip = config_ip->getValue();
            mac = config_mac->getValue();
            port = config_port->getValue();
            user_vec = config_user_vec->getValue();

            // auto it = config_int_vec->getValue();
            // for (auto i : it)
            // {
            //     std::cout << i << std::endl;
            // }

            // std::cout << config_user->toString() << std::endl;
        }

        std::string toString()
        {
            std::stringstream ss;
            ss << "[ip:" + ip + " " + " mac:" + mac + " " + " port:" + std::to_string(port) + " " + "]\n";
            for (auto i = user_vec.begin(); i != user_vec.end(); i++)
            {
                ss << i->toString() << "\n";
            }
            return ss.str();
        }

    private:
        std::string ip;
        std::string mac;
        int port;
        // 用户列表
        std::vector<User> user_vec;
    };

    // ***************user start****************
    // 定义User类的转换偏特化模板类
    /**
     * 数据类型转换-user偏特化版本
     * string -> user
     */
    template <>
    class LexicalCast<std::string, User>
    {
    public:
        User operator()(const std::string &v)
        {
            // loads the input string as a single YAML document
            YAML::Node node = YAML::Load(v);
            User p;
            p.setName(node["name"].as<std::string>());
            p.setAge(node["age"].as<int>());
            p.setPasswd(node["passwd"].as<std::string>());
            return p;
        }
    };

    /**
     * 数据类型转换-user偏特化版本
     * user -> string
     */
    template <>
    class LexicalCast<User, std::string>
    {
    public:
        std::string operator()(const User &v)
        {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "user to string"
                                             << "\n";
            YAML::Node node;
            node["name"] = v.getName();
            node["age"] = v.getAge();
            node["passwd"] = v.getPasswd();
            std::stringstream ss;
            ss << node;
            return ss.str();
        }
    };

    // ***************user end****************
}

int main()
{
    std::string yaml_path = "/root/c_plus_plus_project/sylar/bin/conf/system.yaml";
    sylar::System system;
    system.LoadfromYaml(yaml_path);
    std::cout << system.toString();
    // 从yaml文件中加载配置
}