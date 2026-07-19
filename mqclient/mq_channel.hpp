#ifndef __M_CLIENT_CHANNEL_H__
#define __M_CLIENT_CHANNEL_H__
#include "muduo/net/TcpConnection.h"
#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_threadpool.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_proto.pb.h"
#include "mq_consumer.hpp"
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

namespace haoping
{
    using ProtobufCodecPtr = std::shared_ptr<ProtobufCodec>;               // 表示 Protobuf 编解码器的智能指针
    using basicConsumeResponsePtr = std::shared_ptr<basicConsumeResponse>; // 表示服务端推送消息响应的智能指针
    using basicCommonResponsePtr = std::shared_ptr<basicCommonResponse>;   // 表示普通请求处理结果响应的智能指针

    class Channel
    {
    public:
        using ptr = std::shared_ptr<Channel>;
        Channel(const muduo::net::TcpConnectionPtr conn, const ProtobufCodecPtr codec)
            : _cid(UUIDHelper::uuid()), _conn(conn), _codec(codec) {}

        ~Channel() { basicCancel(); }

        // 获取信道ID
        std::string cid() { return _cid; }

        // 打开信道
        bool openChannel()
        {
            openChannelRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);

            _codec->send(_conn, req);
            basicCommonResponsePtr resp = waitResponse(rid);
            return resp->ok();
        }

        // 关闭信道
        void closeChannel()
        {
            closeChannelRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);

            _codec->send(_conn, req);
            waitResponse(rid);
            return;
        }

        // 向服务端发送声明交换机请求
        bool declareExchange(const std::string &name,
                             ExchangeType type, bool durable, bool auto_delete,
                             google::protobuf::Map<std::string, std::string> &args)
        {
            // 构造一个声明交换机的请求对象
            declareExchangeRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_exchange_name(name);
            req.set_exchange_type(type);
            req.set_durable(durable);
            req.set_auto_delete(auto_delete);
            req.mutable_args()->swap(args);

            // 向服务器发送请求
            _codec->send(_conn, req);

            // 等待服务器的响应
            basicCommonResponsePtr resp = waitResponse(rid);
            return resp->ok();
        }

        // 向服务端发送删除交换机请求
        void deleteExchange(const std::string &name)
        {
            // 构造一个删除交换机的请求对象
            deleteExchangeRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_exchange_name(name);

            // 向服务器发送请求
            _codec->send(_conn, req);

            // 等待服务器的响应
            waitResponse(rid);
            return;
        }

        // 向服务端发送声明队列请求
        bool declareQueue(const std::string &qname,
                          bool qdurable,
                          bool qexclusive,
                          bool qauto_delete,
                          google::protobuf::Map<std::string, std::string> &qargs)
        {
            // 构造一个声明队列的请求对象
            declareQueueRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_queue_name(qname);
            req.set_durable(qdurable);
            req.set_auto_delete(qauto_delete);
            req.set_exclusive(qexclusive);
            req.mutable_args()->swap(qargs);

            // 向服务器发送请求
            _codec->send(_conn, req);

            // 等待服务器的响应
            basicCommonResponsePtr resp = waitResponse(rid);
            return resp->ok();
        }

        // 向服务端发送删除队列请求
        void deleteQueue(const std::string &qname)
        {
            deleteQueueRequest req;

            std::string rid = UUIDHelper::uuid();

            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_queue_name(qname);

            _codec->send(_conn, req);
            waitResponse(rid);
            return;
        }

        // 向服务端发送队列绑定请求
        bool queueBind(const std::string &ename, const std::string &qname, const std::string &key)
        {
            queueBindRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_exchange_name(ename);
            req.set_queue_name(qname);
            req.set_binding_key(key);

            _codec->send(_conn, req);
            basicCommonResponsePtr resp = waitResponse(rid);
            return resp->ok();
        }

        // 向服务端发送解除队列绑定请求
        void queueUnBind(const std::string &ename, const std::string &qname)
        {
            queueUnBindRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_exchange_name(ename);
            req.set_queue_name(qname);

            _codec->send(_conn, req);
            waitResponse(rid);
            return;
        }

        // 向服务端发送消息发布请求
        void basicPublish(const std::string &ename, BasicProperties *bp, const std::string &body)
        {
            basicPublishRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_body(body);
            req.set_exchange_name(ename);

            if (bp != nullptr)
            {
                req.mutable_properties()->set_id(bp->id());
                req.mutable_properties()->set_delivery_mode(bp->delivery_mode());
                req.mutable_properties()->set_routing_key(bp->routing_key());
            }

            _codec->send(_conn, req);
            waitResponse(rid);
            return;
        }

        // 向服务端发送消息确认请求
        void basicAck(const std::string &msgid)
        {
            if (_consumer.get() == nullptr)
            {
                DLOG("消息确认时，找不到消费者信息！");
                return;
            }

            basicAckRequest req;

            std::string rid = UUIDHelper::uuid();

            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_queue_name(_consumer->qname);
            req.set_message_id(msgid);

            _codec->send(_conn, req);
            waitResponse(rid);
            return;
        }

        // 向服务端发送订阅队列请求
        bool basicConsume(const std::string &consumer_tag, const std::string &queue_name,
                          bool auto_ack, const ConsumerCallback &cb)
        {
            if (_consumer.get() != nullptr)
            {
                DLOG("当前信道已订阅其他队列消息！");
                return false;
            }

            basicConsumeRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_queue_name(queue_name);
            req.set_consumer_tag(consumer_tag);
            req.set_auto_ack(auto_ack);

            _codec->send(_conn, req);
            basicCommonResponsePtr resp = waitResponse(rid);
            if (resp->ok() == false)
            {
                DLOG("添加订阅失败！");
                return false;
            }

            _consumer = std::make_shared<Consumer>(consumer_tag, queue_name, auto_ack, cb);
            return true;
        }

        // 取消当前 Channel 中消费者的队列订阅
        void basicCancel()
        {
            if (_consumer.get() == nullptr)
            {
                return;
            }

            basicCancelRequest req;

            std::string rid = UUIDHelper::uuid();
            req.set_rid(rid);
            req.set_cid(_cid);
            req.set_queue_name(_consumer->qname);
            req.set_consumer_tag(_consumer->tag);

            _codec->send(_conn, req);
            waitResponse(rid);

            _consumer.reset();
            return;
        }

    public:
        // 连接收到基础响应后，向 hash_map 中添加响应
        // 处理服务端返回的普通请求响应
        // resp 中包含请求 ID、信道 ID 和请求处理结果
        // 该函数使用响应中的 rid 作为 key
        void putBasicResponse(const basicCommonResponsePtr &resp)
        {
            std::unique_lock<std::mutex> lock(_mutex);

            // 将响应对象保存到 _basic_resp 响应缓存表中
            _basic_resp.insert(std::make_pair(resp->rid(), resp));

            // 保存完成后通过 _cv 唤醒正在等待该响应的线程
            _cv.notify_all();
        }

        // 连接收到消息推送后，需要通过信道找到对应的消费者对象，通过回调函数进行消息处理
        // 处理服务端主动推送给消费者的消息
        // resp 中包含消费者标签、消息属性和消息正文
        void consume(const basicConsumeResponsePtr &resp)
        {
            // 检查当前 Channel 是否存在对应的 Consumer 对象
            if (_consumer.get() == nullptr)
            {
                DLOG("消息处理时，未找到订阅者信息！");
                return;
            }
            if (_consumer->tag != resp->consumer_tag())
            {
                DLOG("收到的推送消息中的消费者标识，与当前信道消费者标识不一致！");
                return;
            }

            // 找到消费者后，调用 Consumer 中保存的回调函数处理消息
            // 消息推送不属于普通请求响应，因此不会存入 _basic_resp
            _consumer->callback(resp->consumer_tag(), resp->mutable_properties(), resp->body());
        }

    private:
        basicCommonResponsePtr waitResponse(const std::string &rid)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [&rid, this]()
                     { return _basic_resp.find(rid) != _basic_resp.end(); });

            basicCommonResponsePtr basic_resp = _basic_resp[rid];
            _basic_resp.erase(rid);
            return basic_resp;
        }

    private:
        std::string _cid;                   // 信道 channel ID
        Consumer::ptr _consumer;            // 当前信道消费者
        muduo::net::TcpConnectionPtr _conn; // 当前客户端对应的 TCP 连接
        ProtobufCodecPtr _codec;            // Protobuf 编解码器 用于发送响应和推送消息
        std::mutex _mutex;                  // 互斥锁
        std::condition_variable _cv;        // 条件变量

        // key 表示客户端发送请求时生成的请求 ID
        // value 表示服务端返回的普通请求处理结果
        // 发送请求的线程根据 rid 等待并取得属于自己的响应
        // 响应使用完成后需要从 _basic_resp 中删除，防止旧响应一直占用内存
        std::unordered_map<std::string, basicCommonResponsePtr> _basic_resp; // 响应缓存表
    };
}

#endif