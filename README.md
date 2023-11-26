# sylar-zje
C++高性能服务器框架sylar-源码阅读，包含日志模块、配置模块、线程模块、协程模块、协程调度模块等，个人学习使用，学习过程中记录笔记更新至博客。
博客链接：https://blog.csdn.net/qq_44913173/category_12492198.html

## 进度日志
### 日志模块
**功能介绍**：输出日志，可定义日志输出地（使用观察者模式），如控制台、文件。可自定义日志格式，初始化时对日志格式进行解析，类似printf的功能，按照指定格式进行解析，如依次输出时间、线程ID、线程名称、协程ID、日志级别、日志名称、消息（涉及到线程协程相关的信息，目前是固定的值，非实际获取，做完相应模块后再完善）

### 配置模块
**功能介绍**：目前支持定义、声明配置项，使用yaml-cpp作为YAML解析库，从配置文件中加载用户配置，支持基本数据类型、STL容器、自定义复杂数据类型与YAML字符串的相互转换（使用仿函数、偏特化实现），支持配置变更通知(监听器)，与日志系统进行整合。
```c++
// 从yaml中加载进来是YAML::Node形式
// 反序列化：从string加载到对象
fromString():
    string -> YAML::Node -> T
// 序列化：从对象转为string
toString():
    T -> YAML::Node -> string
```
**待完善**:
> 更新配置时应该调用校验方法进行校验，以保证用户不会给配置项设置一个非法的值。

> 应该需要有导出当前配置的功能（已完成日志系统配置导出到yaml文件）。

> 在多线程的情况下，线程不安全。

> 插入新的键值对时，只能通过Lookup查找键的方式，且必须传入value，如果找不到该键就将键值对插入，可以新写一个函数来实现该功能。

### 线程模块


