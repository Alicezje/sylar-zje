cmake_minimum_required (VERSION 2.8)
project (sylar)

message("project source dir: " ${PROJECT_SOURCE_DIR})
# project source dir: /root/c_plus_plus_project/sylar

# add sub dit
add_subdirectory(sylar)

# head file dir
include_directories(${PROJECT_SOURCE_DIR}/sylar)
include_directories(/root/app/yaml/yaml-cpp-0.8.0/include)

# 设置库文件路径
set(SYLAR_LOG_LIB 
    ${PROJECT_SOURCE_DIR}/sylar/log.cc 
    ${PROJECT_SOURCE_DIR}/sylar/util.cc
    ${PROJECT_SOURCE_DIR}/sylar/config.cc
)
add_library(sylar_log_shared SHARED ${SYLAR_LOG_LIB})
add_library(sylar_log_static STATIC ${SYLAR_LOG_LIB})

set_target_properties (sylar_log_shared PROPERTIES OUTPUT_NAME "sylar_log")
set_target_properties (sylar_log_static PROPERTIES OUTPUT_NAME "sylar_log")

set(YAML_LIB_PATH /root/app/yaml/yaml-cpp-0.8.0/build/libyaml-cpp.so)

# 设置静态库文件目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
# 动态库文件目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 可执行文件目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/build/)


find_library(SYLAR_LOG_LIB sylar_log HINTS ${PROJECT_SOURCE_DIR}/lib)

add_executable(main main.c)
add_executable(test_log ${PROJECT_SOURCE_DIR}/tests/test_log.cc)
target_link_libraries(test_log ${SYLAR_LOG_LIB} ${YAML_LIB_PATH})

add_executable(test_config ${PROJECT_SOURCE_DIR}/tests/test_config.cc)
target_link_libraries(test_config ${SYLAR_LOG_LIB} ${YAML_LIB_PATH})

add_executable(test_config_demo ${PROJECT_SOURCE_DIR}/tests/test_config_demo.cc)
target_link_libraries(test_config_demo ${SYLAR_LOG_LIB} ${YAML_LIB_PATH})