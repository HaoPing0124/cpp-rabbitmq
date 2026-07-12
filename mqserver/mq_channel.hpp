#ifndef __M_CHANNEL_H__
#define __M_CHANNEL_H__
#include "muduo/net/TcpConnection.h"
#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_threadpool.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_proto.pb.h"
#include "mq_consumer.hpp"
#include "mq_host.hpp"
#include "mq_route.hpp"

namespace haoping
{
    using ProtobufCodecPtr = std::shared_ptr<ProtobufCodec>;
    using openChannelRequstPtr = std::shared_ptr<openChannelRequst>;
    using closeChannelRequstPtr = std::shared_ptr<closeChannelRequst>;
    using declareExchangeRequestPtr = std::shared_ptr<declareExchangeRequest>;
    using deleteExchangeRequestPtr = std::shared_ptr<deleteExchangeRequest>;
    using declareQueueRequestPtr = std::shared_ptr<declareQueueRequest>;
    using deleteQueueRequestPtr = std::shared_ptr<deleteQueueRequest>;
    using queueBindRequestPtr = std::shared_ptr<queueBindRequest>;
    using queueUnBindRequestPtr = std::shared_ptr<queueUnBindRequest>;
    using basicPublishRequestPtr = std::shared_ptr<basicPublishRequest>;
    using basicAckRequestPtr = std::shared_ptr<basicAckRequest>;
    using basicConsumerRequestPtr = std::shared_ptr<basicConsumerRequest>;
    using basicCancelRequestPtr = std::shared_ptr<basicCancelRequest>;
    using basicConsumerResponsePtr = std::shared_ptr<basicConsumerResponse>;
    using basicCommonResponsePtr = std::shared_ptr<basicCommonResponse>;

    class Channel
    {
    public:
        using ptr = std::shared_ptr<Channel>;
        Channel(const std::string &id,
                const VirtualHost::ptr &host,
                const ConsumerManager::ptr &cmp,
                const ProtobufCodecPtr &codec,
                const muduo::net::TcpConnectionPtr &conn,
                const ThreadPool::ptr &pool)
            : _cid(id), _conn(conn), _codec(codec), _cmp(cmp), _host(host), _pool(pool)
        {
        }

        ~Channel()
        {
            if (_consumer.get() != nullptr)
            {
                _cmp->remove(_consumer->tag, _consumer->qname);
            }
            DLOG("del Channel: %p", this);
        }

        // 交换机的声明与删除
        void declareExchange(const declareExchangeRequestPtr &req)
        {
            bool ret = _host->declareExchange(req->exchang_name(),
                                              req->exchang_type(), req->durable(),
                                              req->auto_delete(), req->args());
            return basicResponse(ret, req->rid(), req->cid());
        }
        void deleteExchange(const deleteExchangeRequestPtr &req)
        {
            _host->deleteExchange(req->exchang_name());
            return basicResponse(true, req->rid(), req->cid());
        }

        // 队列的声明与删除
        void declareQueue(const declareQueueRequestPtr &req)
        {
            bool ret = _host->declareQueue(req->queue_name(),
                                           req->durable(), req->exclusive(),
                                           req->auto_delete(), req->args());

            if (ret == false)
            {
                return basicResponse(false, req->rid(), req->cid());
            }
            _cmp->initQueueConsumer(req->queue_name()); // 初始化队列的消费者管理句柄
            return basicResponse(true, req->rid(), req->cid());
        }
        void deleteQueue(const deleteQueueRequestPtr &req)
        {
            _cmp->destroyQueueConsumer(req->queue_name());
            _host->deleteQueue(req->queue_name());
            return basicResponse(true, req->rid(), req->cid());
        }

        // 队列的绑定与解除绑定
        void queueBind(const queueBindRequestPtr &req)
        {
            bool ret = _host->bind(req->exchang_name(), req->queue_name(), req->binding_key());
            return basicResponse(ret, req->rid(), req->cid());
        }
        void queueUnBind(const queueUnBindRequestPtr &req)
        {
            _host->unBind(req->exchang_name(), req->queue_name());
            return basicResponse(true, req->rid(), req->cid());
        }

        // 消息的发布
        void basicPublish();

        // 消息的确认
        void basicAck();

        // 订阅队列消息
        void basicConsume();

        // 取消订阅
        void basicCancel();

    private:
        void basicResponse(bool ok, const std::string &rid, const std::string &cid)
        {
            basicCommonResponse resp;
            resp.set_ok(ok);
            resp.set_rid(rid);
            resp.set_cid(cid);
            _codec->send(_conn, resp);
        }

    private:
        std::string _cid;                   // 信道channel ID
        Consumer::ptr _consumer;            // 当前信道消费者
        muduo::net::TcpConnectionPtr _conn; // 当前客户端对应的 TCP 连接
        ProtobufCodecPtr _codec;            // Protobuf 编解码器 用于发送响应和推送消息
        ConsumerManager::ptr _cmp;          // 消费者管理器
        VirtualHost::ptr _host;             // 虚拟主机 管理:交换机、队列、绑定关系、消息
        ThreadPool::ptr _pool;              // 异步任务线程池。
    };
}

#endif