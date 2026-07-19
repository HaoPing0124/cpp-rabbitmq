#include "mq_connection.hpp"

int main()
{
    // 1. 实例化异步工作线程对象
    haoping::AsyncWorker::ptr awp = std::make_shared<haoping::AsyncWorker>();

    // 2. 实例化连接对象
    haoping::Connection::ptr conn = std::make_shared<haoping::Connection>("127.0.0.1", 8085, awp);

    // 3. 通过连接创建信道
    haoping::Channel::ptr channel = conn->openChannel();

    // 4. 通过信道提供的服务完成所需
    //   1. 声明一个交换机exchange1
    google::protobuf::Map<std::string, std::string> tmp_map;
    channel->declareExchange("exchange1", haoping::ExchangeType::TOPIC, true, false, tmp_map);

    //  2. 声明一个队列queue1
    channel->declareQueue("queue1", true, false, false, tmp_map);

    //  3. 声明一个队列queue2
    channel->declareQueue("queue2", true, false, false, tmp_map);

    //  4. 绑定queue1-exchange1，且binding_key设置为queue1
    channel->queueBind("exchange1", "queue1", "queue1");

    //  5. 绑定queue2-exchange1，且binding_key设置为news.music.#
    channel->queueBind("exchange1", "queue2", "news.music.#");

    // 5. 循环向交换机发布消息
    for (int i = 0; i < 10; i++)
    {
        haoping::BasicProperties bp;
        bp.set_id(haoping::UUIDHelper::uuid());
        bp.set_delivery_mode(haoping::DeliveryMode::DURABLE);
        bp.set_routing_key("news.music.pop");

        channel->basicPublish("exchange1", &bp, "Hello World-TOPIC" + std::to_string(i));
    }

    haoping::BasicProperties bp;
    bp.set_id(haoping::UUIDHelper::uuid());
    bp.set_delivery_mode(haoping::DeliveryMode::DURABLE);
    bp.set_routing_key("news.music.sport");
    channel->basicPublish("exchange1", &bp, "Hello TOPIC-TEST-1");

    bp.set_routing_key("news.sport.sport");
    channel->basicPublish("exchange1", &bp, "Hello TOPIC-TEST-2");

    // 6. 关闭信道
    conn->closeChannel(channel);
    return 0;
}