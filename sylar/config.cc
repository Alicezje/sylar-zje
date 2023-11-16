#include "config.h"

namespace sylar
{
    // 静态成员变量，需要在外部定义一下
    Config::ConfigVarMap Config::s_datas;

    ConfigVarBase::ptr Config::LookupBase(std::string &name)
    {
        auto it = s_datas.find(name);
        return it == s_datas.end() ? nullptr : it->second;
    }

    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos)
        {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));

        if (node.IsMap()) // 如果为map类型
        {
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                // 遍历map中的每个结点，然后递归解析
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node &root)
    {
        // 结点类型<string,YAML::Node>
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        // 将root中的结点进行解析，存放到all_nodes中
        ListAllMember("", root, all_nodes);

        for (auto &i : all_nodes)
        {
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }

            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = LookupBase(key);

            if (var)
            {
                if (i.second.IsScalar())
                {
                    // 从字符串中加载，设置m_val，进行更新
                    var->fromString(i.second.Scalar());
                }
                else
                {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

}