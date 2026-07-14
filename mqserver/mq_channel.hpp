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
    using openChannelRequestPtr = std::shared_ptr<openChannelRequest>;
    using closeChannelRequestPtr = std::shared_ptr<closeChannelRequest>;
    using declareExchangeRequestPtr = std::shared_ptr<declareExchangeRequest>;
    using deleteExchangeRequestPtr = std::shared_ptr<deleteExchangeRequest>;
    using declareQueueRequestPtr = std::shared_ptr<declareQueueRequest>;
    using deleteQueueRequestPtr = std::shared_ptr<deleteQueueRequest>;
    using queueBindRequestPtr = std::shared_ptr<queueBindRequest>;
    using queueUnBindRequestPtr = std::shared_ptr<queueUnBindRequest>;
    using basicPublishRequestPtr = std::shared_ptr<basicPublishRequest>;
    using basicAckRequestPtr = std::shared_ptr<basicAckRequest>;
    using basicConsumeRequestPtr = std::shared_ptr<basicConsumeRequest>;
    using basicCancelRequestPtr = std::shared_ptr<basicCancelRequest>;
    using basicConsumeResponsePtr = std::shared_ptr<basicConsumeResponse>;
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
            // 如果当前信道已经创建消费者，需要先从消费者管理器中移除
            if (_consumer.get() != nullptr)
            {
                _cmp->remove(_consumer->tag, _consumer->qname);
            }
            DLOG("del Channel: %p", this);
        }

        // 交换机的声明与删除
        void declareExchange(const declareExchangeRequestPtr &req)
        {
            // 从请求中取得交换机名称、类型、持久化等参数
            // 交给虚拟主机完成交换机声明
            bool ret = _host->declareExchange(req->exchange_name(),
                                              req->exchange_type(), req->durable(),
                                              req->auto_delete(), req->args());
            return basicResponse(ret, req->rid(), req->cid());
        }
        void deleteExchange(const deleteExchangeRequestPtr &req)
        {
            _host->deleteExchange(req->exchange_name());
            return basicResponse(true, req->rid(), req->cid());
        }

        // 队列的声明与删除
        void declareQueue(const declareQueueRequestPtr &req)
        {
            // 从请求中取得队列名称、持久化、独占等参数
            // 交给虚拟主机完成队列声明
            bool ret = _host->declareQueue(req->queue_name(),
                                           req->durable(), req->exclusive(),
                                           req->auto_delete(), req->args());

            // 队列声明失败，直接向客户端返回失败响应
            if (ret == false)
            {
                return basicResponse(false, req->rid(), req->cid());
            }

            // 队列声明成功后，为该队列创建消费者管理结构
            _cmp->initQueueConsumer(req->queue_name()); // 初始化队列的消费者管理句柄
            return basicResponse(true, req->rid(), req->cid());
        }
        void deleteQueue(const deleteQueueRequestPtr &req)
        {
            // 删除队列之前，先销毁该队列对应的消费者管理结构
            _cmp->destroyQueueConsumer(req->queue_name());

            // 删除虚拟主机中保存的队列
            _host->deleteQueue(req->queue_name());

            return basicResponse(true, req->rid(), req->cid());
        }

        // 队列的绑定与解除绑定
        void queueBind(const queueBindRequestPtr &req)
        {
            // 将指定队列通过绑定键绑定到指定交换机
            bool ret = _host->bind(req->exchange_name(), req->queue_name(), req->binding_key());

            // 将绑定结果返回给客户端
            return basicResponse(ret, req->rid(), req->cid());
        }
        void queueUnBind(const queueUnBindRequestPtr &req)
        {
            // 解除指定交换机和指定队列之间的绑定关系
            _host->unBind(req->exchange_name(), req->queue_name());

            return basicResponse(true, req->rid(), req->cid());
        }

        // 消息的发布
        void basicPublish(const basicPublishRequestPtr &req)
        {
            // 1.根据客户端给出的交换机名称，查找交换机
            auto ep = _host->selectExchange(req->exchange_name());

            // 如果没有找到交换机，说明发布目标不存在
            if (ep.get() == nullptr)
            {
                // 给客户端回复：发布失败
                basicResponse(false, req->rid(), req->cid());
            }

            // 2.获取这个交换机绑定的所有队列
            MsgQueueBindingMap mqbm = _host->exchangeBindings(req->exchange_name());

            // properties保存消息属性
            // 请求可能没有设置消息属性，所以先设置为空指针
            BasicProperties *properties = nullptr;

            // routing_key保存消息的路由键
            // 如果请求没有属性，routing_key暂时就是空字符串
            std::string routing_key;

            // 判断客户端是否设置了消息属性
            if (req->has_properties())
            {
                // 取得请求对象内部的消息属性对象地址
                properties = req->mutable_properties();

                // 从消息属性中取出routing_key
                routing_key = properties->routing_key();
            }

            // 3.遍历交换机绑定的每一个队列
            for (auto &binding : mqbm)
            {
                // binding.first表示当前绑定的队列名称
                // binding.second表示当前绑定关系对象

                // 判断当前队列是否符合路由条件
                if (Router::route(ep->type, routing_key, binding.second->binding_key))
                {
                    // 4.把消息放进匹配成功的队列
                    _host->basicPublish(binding.first, properties, req->body());

                    // 5.生成一个消费任务
                    // 这个任务将来执行时，实际上调用: this->consume(binding.first)
                    auto task = std::bind(&Channel::consume, this, binding.first);

                    // 将消费任务交给线程池
                    _pool->push(task);
                }
            }
            // 给生产者客户端回复：发布请求处理完成
            return basicResponse(true, req->rid(), req->cid());
        }

        // 消息的确认
        void basicAck(const basicAckRequestPtr &req)
        {
            // 根据队列名称和消息ID确认消息
            // 虚拟主机收到确认后会删除对应的待确认消息
            _host->basicAck(req->queue_name(), req->message_id());

            return basicResponse(true, req->rid(), req->cid());
        }

        // 订阅队列消息
        void basicConsume(const basicConsumeRequestPtr &req)
        {
            // 1. 判断队列是否存在
            bool ret = _host->existsQueue(req->queue_name());
            if (ret == false)
            {
                return basicResponse(false, req->rid(), req->cid());
            }

            // 2. 创建队列的消费者
            // 将Channel::callback包装成消费者收到消息后的回调函数
            // 三个占位符分别对应 1.消费者标签 2.消息属性 3.消息正文
            auto cb = std::bind(&Channel::callback, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3);

            // 创建了消费者之后，当前的channel角色就是个消费者
            _consumer = _cmp->create(req->consumer_tag(), req->queue_name(), req->auto_ack(), cb);

            // 消费者创建失败，向客户端返回失败响应
            if (_consumer.get() == nullptr)
            {
                return basicResponse(false, req->rid(), req->cid());
            }

            return basicResponse(true, req->rid(), req->cid());
        }

        // 取消订阅
        void basicCancel(const basicCancelRequestPtr &req)
        {
            // 根据消费者标签和队列名称删除消费者
            _cmp->remove(req->consumer_tag(), req->queue_name());

            return basicResponse(true, req->rid(), req->cid());
        }

    private:
        // 消费者收到消息后执行的回调函数
        // 负责把服务端队列中的消息封装成响应并推送给客户端
        // 参数：1.tag 表示消费者标签 2.bp 表示消息属性 3.body 表示消息正文
        void callback(const std::string tag, const BasicProperties *bp, const std::string &body)
        {
            // 创建服务端向消费者推送消息的响应对象
            basicConsumeResponse resp;

            resp.set_cid(_cid);
            resp.set_consumer_tag(tag);
            resp.set_body(body);

            if (bp)
            {
                resp.mutable_properties()->set_id(bp->id());
                resp.mutable_properties()->set_delivery_mode(bp->delivery_mode());
                resp.mutable_properties()->set_routing_key(bp->routing_key());
            }
            _codec->send(_conn, resp);
        }

        // 执行一次指定队列的消息消费任务
        void consume(const std::string &qname)
        {
            // 指定队列消费消息
            // 1. 从队列中取出一条消息
            MessagePtr mp = _host->basicConsume(qname);
            if (mp.get() == nullptr)
            {
                DLOG("执行消费任务失败，%s 队列没有消息！", qname.c_str());
                return;
            }

            // 2. 从队列订阅者中取出一个订阅者
            Consumer::ptr cp = _cmp->choose(qname);
            if (cp.get() == nullptr)
            {
                DLOG("执行消费任务失败，%s 队列没有消费者！", qname.c_str());
                return;
            }

            // 3. 调用订阅者对应的消息处理函数，实现消息的推送
            cp->callback(cp->tag, mp->mutable_payload()->mutable_properties(), mp->payload().body());

            // 4. 判断如果订阅者是自动确认---不需要等待确认，直接删除消息，否则需要外部收到消息确认后再删除
            if (cp->auto_ack)
                _host->basicAck(qname, mp->payload().properties().id());
        }

        // 向客户端发送通用处理结果响应
        // 1.ok表示请求处理是否成功
        // 2.rid表示请求ID，用于让客户端找到对应的请求
        // 3.cid表示信道ID，用于区分请求属于哪个信道
        void basicResponse(bool ok, const std::string &rid, const std::string &cid)
        {
            // 创建通用响应对象
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

    // 信道管理器
    // 负责统一创建、保存、查找和关闭当前连接中的所有Channel对象
    class ChannelManager
    {
    public:
        using ptr = std::shared_ptr<ChannelManager>;
        ChannelManager() {}

        // 打开信道
        // id 表示需要创建的信道ID
        // host 用于管理交换机、队列、绑定关系和消息
        // cmp 用于管理消费者
        // codec 用于发送Protobuf响应
        // conn 表示当前客户端的TCP连接
        // pool 用于异步执行消息消费任务
        bool openChannel(const std::string &id,
                         const VirtualHost::ptr &host,
                         const ConsumerManager::ptr &cmp,
                         const ProtobufCodecPtr &codec,
                         const muduo::net::TcpConnectionPtr &conn,
                         const ThreadPool::ptr &pool)
        {
            std::unique_lock<std::mutex> lock(_mutex);

            // 根据信道ID查找信道是否已经存在
            auto it = _channels.find(id);
            if (it != _channels.end())
            {
                DLOG("信道：%s 已存在!", id.c_str());
                return false;
            }

            Channel::ptr channel = std::make_shared<Channel>(id, host, cmp, codec, conn, pool);
            _channels.insert(std::make_pair(id, channel));
            return true;
        }

        // 关闭信道
        void closeChannel(const std::string &id)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _channels.erase(id);
        }

        // 根据信道ID获取对应的Channel对象
        Channel::ptr getChannel(const std::string &id)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _channels.find(id);
            if (it == _channels.end())
            {
                return Channel::ptr();
            }

            // 返回找到的Channel智能指针
            // 即使函数结束后锁被释放，返回的智能指针仍然能够保证Channel对象存活
            return it->second;
        }

    private:
        std::mutex _mutex;
        std::unordered_map<std::string, Channel::ptr> _channels;
    };
}

#endif