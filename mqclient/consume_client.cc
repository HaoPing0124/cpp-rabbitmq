#include "mq_connection.hpp"

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace
{
struct DemoConfig
{
    std::string exchange_name;
    haoping::ExchangeType exchange_type;
    std::string queue1_binding;
    std::string queue2_binding;
};

bool loadConfig(const std::string &mode, DemoConfig *config)
{
    if (mode == "direct")
    {
        *config = {"exchange1", haoping::ExchangeType::DIRECT, "orange", "black"};
        return true;
    }
    if (mode == "fanout")
    {
        *config = {"exchange1", haoping::ExchangeType::FANOUT, "queue1", "queue2"};
        return true;
    }
    if (mode == "topic")
    {
        *config = {"exchange1", haoping::ExchangeType::TOPIC, "queue1", "news.music.#"};
        return true;
    }
    return false;
}

void declareTopology(const haoping::Channel::ptr &channel, const DemoConfig &config)
{
    google::protobuf::Map<std::string, std::string> arguments;
    channel->declareExchange(config.exchange_name, config.exchange_type, true, false, arguments);
    channel->declareQueue("queue1", true, false, false, arguments);
    channel->declareQueue("queue2", true, false, false, arguments);
    channel->queueBind(config.exchange_name, "queue1", config.queue1_binding);
    channel->queueBind(config.exchange_name, "queue2", config.queue2_binding);
}

void consumeCallback(const haoping::Channel::ptr &channel, const std::string consumer_tag,
                     const haoping::BasicProperties *properties, const std::string &body)
{
    std::cout << consumer_tag << "消费了消息：" << body << std::endl;
    channel->basicAck(properties->id());
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        return 1;
    }

    const std::string mode = argv[1];
    const std::string queue_name = argv[2];
    if (queue_name != "queue1" && queue_name != "queue2")
    {
        return 1;
    }

    DemoConfig config;
    if (!loadConfig(mode, &config))
    {
        return 1;
    }

    haoping::AsyncWorker::ptr worker = std::make_shared<haoping::AsyncWorker>();
    haoping::Connection::ptr connection = std::make_shared<haoping::Connection>("127.0.0.1", 8085, worker);
    haoping::Channel::ptr channel = connection->openChannel();
    declareTopology(channel, config);

    auto callback = std::bind(consumeCallback, channel, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3);
    channel->basicConsume("consumer1", queue_name, false, callback);

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}
