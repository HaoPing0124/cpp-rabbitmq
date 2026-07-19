cmake_minimum_required(VERSION 3.16)

project(cpp_rabbitmq LANGUAGES CXX)

# 统一使用 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 没有主动指定构建类型时，默认使用 Debug
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# 所有可执行文件统一生成到 build/bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(Threads REQUIRED)

# 项目头文件路径
include_directories(
    ${CMAKE_SOURCE_DIR}/mqcommon
    ${CMAKE_SOURCE_DIR}/mqserver
    ${CMAKE_SOURCE_DIR}/mqclient
    ${CMAKE_SOURCE_DIR}/mqthird/include
)

# 项目内置 muduo 库路径
link_directories(
    ${CMAKE_SOURCE_DIR}/mqthird/lib
)

# Protobuf 协议与 muduo Protobuf 编解码模块
add_library(mq_protocol STATIC
    mqcommon/mq_msg.pb.cc
    mqcommon/mq_proto.pb.cc
    mqthird/include/muduo/proto/codec.cc
)

target_link_libraries(mq_protocol
    PUBLIC
        protobuf
        z
        Threads::Threads
)

# 项目公共链接库
set(MQ_COMMON_LIBS
    mq_protocol
    muduo_net
    muduo_base
    sqlite3
    protobuf
    z
    Threads::Threads
)

# Broker 服务端
add_executable(mqserver
    mqserver/mq_server.cc
)

target_link_libraries(mqserver
    PRIVATE
        ${MQ_COMMON_LIBS}
)

# 生产者客户端
add_executable(publish_client
    mqclient/publish_client.cc
)

target_link_libraries(publish_client
    PRIVATE
        ${MQ_COMMON_LIBS}
)

# 消费者客户端
add_executable(consume_client
    mqclient/consume_client.cc
)

target_link_libraries(consume_client
    PRIVATE
        ${MQ_COMMON_LIBS}
)

# 开启 CTest
enable_testing()

# 添加测试程序的公共函数
function(add_mq_test target_name source_file)
    add_executable(${target_name}
        ${source_file}
    )

    target_link_libraries(${target_name}
        PRIVATE
            ${MQ_COMMON_LIBS}
            gtest
    )

    # 每个测试使用独立运行目录，避免测试数据互相干扰
    file(MAKE_DIRECTORY
        ${CMAKE_BINARY_DIR}/test_runtime/${target_name}
    )

    add_test(
        NAME ${target_name}
        COMMAND $<TARGET_FILE:${target_name}>
    )

    set_tests_properties(${target_name}
        PROPERTIES
            WORKING_DIRECTORY
            ${CMAKE_BINARY_DIR}/test_runtime/${target_name}
    )
endfunction()

# 工具模块测试
add_mq_test(mq_filetest
    mqtest/mq_filetest.cpp
)

# 交换机模块测试
add_mq_test(mq_exchangetest
    mqtest/mq_exchangetest.cpp
)

# 队列模块测试
add_mq_test(mq_queuetest
    mqtest/mq_queuetest.cpp
)

# 消息模块测试
add_mq_test(mq_msgtest
    mqtest/mq_msgtest.cpp
)

# 虚拟主机模块测试
add_mq_test(mq_hosttest
    mqtest/mq_hosttest.cpp
)

# 路由模块测试
add_mq_test(mq_routetest
    mqtest/mq_routetest.cpp
)

# 消费者模块测试
add_mq_test(mq_consumertest
    mqtest/mq_consumertest.cpp
)

# Channel 模块编译测试
add_mq_test(mq_channeltest
    mqtest/mq_channeltest.cpp
)

# Connection 模块编译测试
add_mq_test(mq_connectiontest
    mqtest/mq_connectiontest.cpp
)
