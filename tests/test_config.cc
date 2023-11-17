#include <string>
#include "config.h"
#include "log.h"
#include "util.h"
#include <yaml-cpp/yaml.h>

void print_yaml(const YAML::Node &node, int level)
{
    /**
     * node.Type()
     * enum value { Undefined, Null, Scalar, Sequence, Map };
     */
    if (node.IsScalar())
    {
        std::cout << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << std::endl;
    }
    else if (node.IsNull())
    {
        std::cout << std::string(level * 4, ' ') << "NULL"
                  << " - " << node.Type() << std::endl;
    }
    else if (node.IsMap())
    {
        // 如果类型为map还要继续向下遍历
        for (auto it = node.begin(); it != node.end(); it++)
        {
            // 输出键和值的类型
            std::cout << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << std::endl;
            // 继续遍历值
            print_yaml(it->second, level + 1);
        }
    }
    else if (node.IsSequence())
    {
        for (size_t i = 0; i < node.size(); i++)
        {
            std::cout << std::string(level * 4, ' ') << i << " - " << node[i].Type() << std::endl;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml()
{
    // YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;

    YAML::Node node;
    assert(node.IsNull()); // 初始化的节点是Null类型
    node["key"] = "value"; // 当你给它赋值键值对，它转变为Map类型
    // node.force_insert("key", "value");//这个操作和上面等价，但是它不会检查是否存在"key"键，不推荐使用
    if (node["mascot"])
        std::cout << node["mascot"].as<std::string>() << "\n"; // 单纯的查询操作不会增加一个key，当然上面的if不会执行

    node["number"] = 255;
    assert(node.IsMap()); // node是一个Map
    node["seq"].push_back("first element");
    node["seq"].push_back("second element"); // node的seq下是Sequence类型，有两个参数

    YAML::Node node_2;
    node_2.push_back("first item"); // 如果你不给node_2键值对，它是一个sequence类型
    node_2.push_back("second_item");
    node_2.push_back("third_item");

    std::vector<int> v = {1, 3, 5, 7, 9}; // 给node_2插入了一个Sequence
    node_2.push_back(v);
    assert(node_2.IsSequence()); // 当然，node_2仍然是一个Sequence

    assert(node_2[0].as<std::string>() == "first item");

    // 对于Sequence类型，你可以使用它的下标来访问
    // 注意这里as<T>是一个模板转换，node_2[0]的type是NodeType::Scalar

    auto it = node_2.begin();
    for (; it != node_2.end(); it++)
        std::cout << *(it) << std::endl;

    // 当然，你也可以用迭代器来访问
    // 他们的类型分别是NodeType::Scalar，NodeType::Scalar，NodeType::Scalar，NodeType::Sequence
    // 取值时记得使用as进行模板转换
    node_2["key"] = "value";
    assert(node_2.IsMap());                                                        // 一旦node_2接收到键值对，它转变为Map类型
    assert(node_2[0].as<std::string>() == "first item");                           // 此时，Sequence时的下标变为它的key值
    node["node_2"] = node_2;                                                       // 将node_2作为node的一个子项
    node["pointer_to_first_element"] = node["seq"][0];                             // 你也可以给已有的node设置一个别名，类似于一个指针
    assert(node["pointer_to_first_element"].as<std::string>() == "first element"); // 你可以通过这个指针访问那个node

    node.remove(node["seq"][0]);             // 你可以通过指定一个node来删除它
    node.remove("pointer_to_first_element"); // 你也可以通过指定key来删除它
}

void test_config(sylar::ConfigVar<int>::ptr g_int_value_config)
{
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->toString();

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->toString();
}

void test_vector_to_string()
{
    // sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
    //     sylar::Config::Lookup("test", std::vector<int>{1, 2}, "system int vec");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
        sylar::Config::Lookup<std::vector<int>>("test");

    // g_int_vec_value_config->getValue();
    // auto v = g_int_vec_value_config->getValue();
    // for (auto i : v)
    // {
    //     SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i;
    // }
}

void test_list_to_string()
{
    sylar::ConfigVar<std::list<int>>::ptr g_int_vec_value_config =
        sylar::Config::Lookup("test", std::list<int>{1, 2}, "system int vec");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    auto v = g_int_vec_value_config->getValue();
    for (auto i : v)
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i;
    }
}

void test_set_to_int()
{
    std::set<int> s;
    s.insert(1);
    s.insert(2);
    sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config =
        sylar::Config::Lookup("system.int_set", s, "system int set");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    auto v = g_int_set_value_config->getValue();
    for (auto i : v)
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i;
    }
}

void test_unordered_set_to_int()
{
    std::unordered_set<int> s;
    s.insert(1);
    s.insert(2);
    sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_unordered_set_value_config =
        sylar::Config::Lookup("system.int_unordered_set", s, "system int unordered_set");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    auto v = g_int_unordered_set_value_config->getValue();
    for (auto i : v)
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << i;
    }
}

void test_map_to_int()
{
    std::map<std::string, int> s;
    s["key"] = 10;
    sylar::ConfigVar<std::map<std::string, int>>::ptr g_int_unordered_set_value_config =
        sylar::Config::Lookup("system.int_map", s, "system int map");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    std::map<std::string, int> v = g_int_unordered_set_value_config->getValue();
    std::map<std::string, int>::iterator it;
    for (it = v.begin(); it != v.end(); it++)
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << it->first << " " << it->second;
    }
}

void test_unordered_map_to_int()
{
    std::unordered_map<std::string, int> s;
    s["key"] = 10;
    sylar::ConfigVar<std::unordered_map<std::string, int>>::ptr g_int_unordered_map_value_config =
        sylar::Config::Lookup("system.int_map", s, "system int map");

    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/log.yaml");
    // 从yaml文件加载到ConfigVarMap s_datas中
    sylar::Config::LoadFromYaml(root);

    std::unordered_map<std::string, int> v = g_int_unordered_map_value_config->getValue();
    std::unordered_map<std::string, int>::iterator it;
    for (it = v.begin(); it != v.end(); it++)
    {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << it->first << " " << it->second;
    }
}

void test_load_ListAllMember()
{
    YAML::Node root = YAML::LoadFile("/root/c_plus_plus_project/sylar/bin/conf/test.yaml");

    // print_yaml(root, 0);
    sylar::ConfigVar<int>::ptr g_int_vec_value_config =
        sylar::Config::Lookup("user.number", 1, "user number");

    sylar::Config::LoadFromYaml(root);
}

int main()
{
    std::cout << "hello world" << std::endl;
    // sylar::ConfigVar<int>::ptr g_int_value_config = sylar::Config::Lookup("system.port", (int)8080, "system port");

    // std::cout << g_int_value_config->getName() << std::endl;
    // std::cout << g_int_value_config->getValue() << std::endl;
    // std::cout << g_int_value_config->toString() << std::endl;

    // sylar::Logger::ptr t = sylar::LoggerMgr::GetInstance()->getRoot();
    // std::cout << t->getName() << std::endl;

    // SYLAR_LOG_INFO(t)<<"raw string test success";

    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "raw string test success";
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
    // SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();

    // auto res = sylar::Config::Lookup("system.port", (int)80, "system port");

    // std::cout << res->getName() << std::endl;
    // std::cout << res->getValue() << std::endl;

    // sylar::ConfigVar<std::string>::ptr pointer = std::ConfigVar<std::string>("system.address", "192.168.111.110");
    // test_yaml();

    // test_config(g_int_value_config);

    // test_vector_to_string();
    // test_list_to_string();
    // test_set_to_int();
    // test_unordered_set_to_int();
    // test_map_to_int();
    // test_unordered_map_to_int();

    test_load_ListAllMember();

    return 0;
}