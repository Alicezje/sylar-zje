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
set(SYLAR_LIB 
    ${PROJECT_SOURCE_DIR}/sylar/log.cc  
    ${PROJECT_SOURCE_DIR}/sylar/log_socket.cc
    ${PROJECT_SOURCE_DIR}/sylar/thread.cc
    ${PROJECT_SOURCE_DIR}/sylar/mutex.cc
    ${PROJECT_SOURCE_DIR}/sylar/config.cc
    ${PROJECT_SOURCE_DIR}/sylar/util.cc
    ${PROJECT_SOURCE_DIR}/sylar/fiber.cc
    ${PROJECT_SOURCE_DIR}/sylar/scheduler.cc
)
add_library(sylar_lib_shared SHARED ${SYLAR_LIB})
add_library(sylar_lib_static STATIC ${SYLAR_LIB})
set_target_properties (sylar_lib_shared PROPERTIES OUTPUT_NAME "sylar_lib")
set_target_properties (sylar_lib_static PROPERTIES OUTPUT_NAME "sylar_lib")

set(YAML_LIB_PATH /root/app/yaml/yaml-cpp-0.8.0/build/libyaml-cpp.so)
set(PTHREAD_LIB pthread)

# 设置静态库文件目录
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
# 动态库文件目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
# 可执行文件目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/build/)

find_library(SYLAR_LIB sylar_lib HINTS ${PROJECT_SOURCE_DIR}/lib)

# add_executable(main main.c)

# add_executable(test_log ${PROJECT_SOURCE_DIR}/tests/test_log.cc)
# target_link_libraries(test_log ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_config ${PROJECT_SOURCE_DIR}/tests/test_config.cc)
# target_link_libraries(test_config ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_config_demo ${PROJECT_SOURCE_DIR}/tests/test_config_demo.cc)
# target_link_libraries(test_config_demo ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_thread ${PROJECT_SOURCE_DIR}/tests/test_thread.cc)
# target_link_libraries(test_thread ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_thread_consumer ${PROJECT_SOURCE_DIR}/tests/test_thread_consumer.cc)
# target_link_libraries(test_thread_consumer ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_semaphore ${PROJECT_SOURCE_DIR}/tests/test_semaphore.cc)
# target_link_libraries(test_semaphore ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_util ${PROJECT_SOURCE_DIR}/tests/test_util.cc)
# target_link_libraries(test_util ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

add_executable(test_fiber ${PROJECT_SOURCE_DIR}/tests/test_fiber.cc)
target_link_libraries(test_fiber ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_log_socket ${PROJECT_SOURCE_DIR}/tests/test_log_socket.cc)
# target_link_libraries(test_log_socket ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(start_log_socket_server ${PROJECT_SOURCE_DIR}/tests/start_log_socket_server.cc)
# target_link_libraries(start_log_socket_server ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

# add_executable(test_log_send ${PROJECT_SOURCE_DIR}/tests/test_log_send.cc)
# target_link_libraries(test_log_send ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})

add_executable(test_scheduler ${PROJECT_SOURCE_DIR}/tests/test_scheduler.cc)
target_link_libraries(test_scheduler ${SYLAR_LIB} ${YAML_LIB_PATH} ${PTHREAD_LIB})