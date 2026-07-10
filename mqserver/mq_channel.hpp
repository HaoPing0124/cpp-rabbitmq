#ifndef __M_CHANNEL_H__
#define __M_CHANNEL_H__
#include "muduo/net/TcpConnection.h"
#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "mq_consumer.hpp"
#include "mq_host.hpp"
#include "mq_route.hpp"

namespace haoping
{
    using ProtobufCodecPtr = std::shared_ptr<ProtobufCodec>;
    class Channel
    {
    public:
        using ptr = std::shared_ptr<Channel>;
        Channel(const std::string &id,
                const VirtualHost::ptr &host,
                const ConsumerManager::ptr &cmp,
                const ProtobufCodecPtr &codec,
                const muduo::net::TcpConnectionPtr &conn)
            : _cid(id), _conn(conn), _codec(codec), _cmp(cmp), _host(host)
        {
        }

        // 交换机的声明与删除
        void declareExchange();
        void deleteExchange();

        // 队列的声明与删除
        void declareQueue();
        void deleteQueue();

        // 队列的绑定与解除绑定
        void queueBind();
        void queueUnBind();

        // 消息的发布
        void basicPublish();

        // 消息的确认
        void basicAck();

        // 订阅队列消息
        void basicConsume();

        // 取消订阅
        void basicCancel();

    private:
        std::string _cid;
        Consumer::ptr _consumer;
        muduo::net::TcpConnectionPtr _conn;
        ProtobufCodecPtr _codec;
        ConsumerManager::ptr _cmp;
        VirtualHost::ptr _host;
    };
}

#endif