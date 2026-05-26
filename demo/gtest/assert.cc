/*
    断言宏的使用
        ASSERT_ 断言失败则退出
        EXPECT_ 断言失败继续执行
    使用规范
        断言宏 必须在单元测试宏函数中使用
*/

#include <iostream>
#include <gtest/gtest.h>

TEST(test, less_than)
{
    int age = 20;
    EXPECT_LT(age, 18); // 测试 age < 18 是否为真
    std::cout << "less_than测试完毕" << std::endl;
}

TEST(test, great_than)
{
    int age = 20;
    EXPECT_GT(age, 18); // 测试 age > 18 是否为真
    std::cout << "great_than测试完毕" << std::endl;
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
