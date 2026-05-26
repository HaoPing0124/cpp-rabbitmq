#include <iostream>
#include <gtest/gtest.h>
#include <unordered_map>

class MyTest : public testing::Test
{
public:
    static void SetUpTestCase()
    {
        // 假设是全局数据测试容器，将在这插入公共的测试数据
        std::cout << "测试单元前执行,初始化总环境" << std::endl;
    }

    static void TearDownTestCase()
    {
        // 清理公共的测试数据
        std::cout << "所有测试单元执行完毕 清理总环境" << std::endl;
    }

    virtual void SetUp() override
    {
        // 将在这插入每个测试单元独立的测试数据
        std::cout << "测试单元执行前的环境初始化" << std::endl;
        _mp.insert(std::make_pair("hello", "你好"));
        _mp.insert(std::make_pair("bye", "再见"));
    }

    virtual void TearDown() override
    {
        // 在这里清理每个测试单元自己插入的数据
        std::cout << "测试单元执行完毕后的环境清理" << std::endl;
        _mp.clear();
    }

public:
    std::unordered_map<std::string, std::string> _mp;
};

// 必须TEST_F 类名必须相同
TEST_F(MyTest, insert_test)
{
    // 每个单元测试里的_mp都是独立的
    _mp.insert(std::make_pair("apple", "苹果"));
    _mp.insert(std::make_pair("banana", "香蕉"));
    ASSERT_EQ(_mp.size(), 4);
}

TEST_F(MyTest, test2)
{
    ASSERT_EQ(_mp.size(), 2);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    // testing::AddGlobalTestEnvironment(new MyTest);
    return RUN_ALL_TESTS();
}