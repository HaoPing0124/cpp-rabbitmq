#include <iostream>
#include <gtest/gtest.h>
#include <unordered_map>

class MyEnvironment : public testing::Environment
{
public:
    virtual void SetUp() override
    {
        std::cout << "测试单元执行前的环境初始化" << std::endl;
    }

    virtual void TearDown() override
    {
        std::cout << "测试单元执行完毕后的环境清理" << std::endl;
    }
};

TEST(MyEnvironment, test1)
{
    std::cout << "单元测试1\n";
}

TEST(MyEnvironment, test2)
{
    std::cout << "单元测试2\n";
}

std::unordered_map<std::string, std::string> mp;
class MyMapTest : public testing::Environment
{
public:
    virtual void SetUp() override
    {
        std::cout << "测试单元执行前的环境初始化" << std::endl;
        mp.insert(std::make_pair("hello", "你好"));
        mp.insert(std::make_pair("bye", "再见"));
    }

    virtual void TearDown() override
    {
        std::cout << "测试单元执行完毕后的环境清理" << std::endl;
        mp.clear();
    }
};

TEST(MyMapTest, test1)
{
    ASSERT_EQ(mp.size(), 2);
    mp.erase("hello");
}

TEST(MyMapTest, test2)
{
    ASSERT_EQ(mp.size(), 2);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new MyEnvironment);
    return RUN_ALL_TESTS();
}