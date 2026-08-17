#include "mq_connection.hpp"

#include <chrono>
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

void publishMessage(const haoping::Channel::ptr &channel, const std::string &exchange,
                    const std::string &routing_key, const std::string &body)
{
    haoping::BasicProperties properties;
    properties.set_id(haoping::UUIDHelper::uuid());
    properties.set_delivery_mode(haoping::DeliveryMode::DURABLE);
    properties.set_routing_key(routing_key);
    channel->basicPublish(exchange, &properties, body);
}
} // namespace

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        return 1;
    }

    const std::string mode = argv[1];
    DemoConfig config;
    if (!loadConfig(mode, &config))
    {
        return 1;
    }

    haoping::AsyncWorker::ptr worker = std::make_shared<haoping::AsyncWorker>();
    haoping::Connection::ptr connection = std::make_shared<haoping::Connection>("127.0.0.1", 8085, worker);
    haoping::Channel::ptr channel = connection->openChannel();
    declareTopology(channel, config);

    if (mode == "direct")
    {
        publishMessage(channel, config.exchange_name, "orange", "DIRECT orange message");
        publishMessage(channel, config.exchange_name, "black", "DIRECT black message");
        publishMessage(channel, config.exchange_name, "green", "DIRECT green message");
    }
    else if (mode == "fanout")
    {
        publishMessage(channel, config.exchange_name, "ignored.one", "FANOUT broadcast message 1");
        publishMessage(channel, config.exchange_name, "ignored.two", "FANOUT broadcast message 2");
    }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            publishMessage(channel, config.exchange_name, "news.music.pop",
                           "Hello World-TOPIC" + std::to_string(i));
        }
        publishMessage(channel, config.exchange_name, "news.music.sport", "Hello TOPIC-TEST-1");
        publishMessage(channel, config.exchange_name, "news.sport.sport", "Hello TOPIC-TEST-2");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    connection->closeChannel(channel);
    return 0;
}
