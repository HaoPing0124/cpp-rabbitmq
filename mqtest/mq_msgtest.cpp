#include "../mqserver/mq_message.hpp"
#include <gtest/gtest.h>

haoping::MessageManager::ptr mmp;

class MessageTest : public testing::Environment {
    public:
        virtual void SetUp() override {
            mmp = std::make_shared<haoping::MessageManager>("./data/message/");
            mmp->initQueueMessage("queue1");
        }
        virtual void TearDown() override {
            mmp->clear();
        }
};