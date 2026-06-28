#include "../mqserver/mq_message.hpp"
#include <gtest/gtest.h>

haoping::MessageManager::ptr mmp;

class MessageTest : public testing::Environment
{
public:
    virtual void SetUp() override
    {
        mmp = std::make_shared<haoping::MessageManager>("./data/message/");
        mmp->initQueueMessage("queue1");
    }
    virtual void TearDown() override
    {
        mmp->clear();
    }
};

// 新增消息测试：新增消息，然后观察可获取消息数量，以及持久化消息数量
TEST(message_test, insert_test)
{
    haoping::BasicProperties properties;
    properties.set_id(haoping::UUIDHelper::uuid());
    properties.set_delivery_mode(haoping::DeliveryMode::DURABLE);
    properties.set_routing_key("news.music.pop");
    mmp->insert("queue1", &properties, "Hello World-1", haoping::DeliveryMode::DURABLE);
    mmp->insert("queue1", nullptr, "Hello World-2", haoping::DeliveryMode::DURABLE);
    mmp->insert("queue1", nullptr, "Hello World-3", haoping::DeliveryMode::DURABLE);
    mmp->insert("queue1", nullptr, "Hello World-4", haoping::DeliveryMode::DURABLE);
    mmp->insert("queue1", nullptr, "Hello World-5", haoping::DeliveryMode::UNDURABLE);
    ASSERT_EQ(mmp->getable_count("queue1"), 5);
    ASSERT_EQ(mmp->total_count("queue1"), 4);
    ASSERT_EQ(mmp->durable_count("queue1"), 4);
    ASSERT_EQ(mmp->waitack_count("queue1"), 0);
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new MessageTest);
    return RUN_ALL_TESTS();
}