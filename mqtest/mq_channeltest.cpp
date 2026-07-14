#include "../mqserver/mq_channel.hpp"

int main()
{
    haoping::ChannelManager::ptr cmp = std::make_shared<haoping::ChannelManager>();

    cmp->openChannel("c1",
                     std::make_shared<haoping::VirtualHost>("host1", "./data/host1/message/", "./data/host1/host1.db"),
                     std::make_shared<haoping::ConsumerManager>(),
                     haoping::ProtobufCodecPtr(),
                     muduo::net::TcpConnectionPtr(),
                     ThreadPool::ptr());
    return 0;
}