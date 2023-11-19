#include "config.h"

namespace sylar
{
    // 静态成员变量，需要在外部定义一下
    Config::ConfigVarMap Config::s_datas;

    ConfigVarBase::ptr Config::LookupBase(const std::string &name)
    {
        auto it = s_datas.find(name);
        // 如果找到的返回其值，找不到返回nullptr
        return it == s_datas.end() ? nullptr : it->second;
    }

    static void ListAllMember(const std::string &prefix,
                              const YAML::Node &node,
                              std::list<std::pair<std::string, const YAML::Node>> &output)
    {
        if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos)
        {
            // 字符串prefix中存在字符与上述字符均不匹配，即不合法字符
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));

        // 仅将map类型继续进行解析,其他类型无须继续解析
        if (node.IsMap())
        {
            // 遍历map
            for (auto it = node.begin(); it != node.end(); ++it)
            {
                // 遍历map中的每个结点，然后递归解析
                // if prefix为空字符串，则使用当前遍历结点的key
                // else 使用 prefix+"."+当前遍历结点的key
                // 使用"."表示yaml相关的层级关系
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
            // 遍历，获取key，查找是否包含key，如果包含，将之前修改为从文件中加载的值
            std::string key = i.first;
            if (key.empty())
            {
                continue;
            }
            // 将key转为小写
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            // 查询是否包含key
            ConfigVarBase::ptr var = LookupBase(key);
            // 如果存在key才从文件中加载更新，不存在直接跳过
            if (var)
            {
                if (i.second.IsScalar())
                {
                    // 将YAML::内结点值转为Scalar类型
                    // 然后从字符串中加载（已通过实现偏特化实现了类型的转换），设置m_val，进行更新
                    var->fromString(i.second.Scalar());
                }
                else
                {
                    // 其他类型,偏特化中fromString有对应的处理方法
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

}