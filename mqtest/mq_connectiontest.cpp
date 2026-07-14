#include "../mqserver/mq_connection.hpp"

int main()
{
    haoping::ConnectionManager::ptr connmp = std::make_shared<haoping::ConnectionManager>();
    connmp->newConnection(std::make_shared<haoping::VirtualHost>("host1", "./data/host1/message/", "./data/host1/host1.db"),
                        std::make_shared<haoping::ConsumerManager>(),
                        haoping::ProtobufCodecPtr(),
                        muduo::net::TcpConnectionPtr(),
                        ThreadPool::ptr());
    return 0;
}