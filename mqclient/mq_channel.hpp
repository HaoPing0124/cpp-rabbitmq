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
        Channel();
        ~Channel();

        // 向服务端发送声明交换机请求
        void declareExchange(const std::string &name,
                             ExchangeType type, bool durable, bool auto_delete,
                             const google::protobuf::Map<std::string, std::string> &args);

        // 向服务端发送删除交换机请求
        void deleteExchange(const std::string &name);

        // 向服务端发送声明队列请求
        void declareQueue(const std::string &qname,
                          bool qdurable,
                          bool qexclusive,
                          bool qauto_delete,
                          const google::protobuf::Map<std::string, std::string> &qargs);

        // 向服务端发送删除队列请求
        void deleteQueue(const std::string &qname);

        // 向服务端发送队列绑定请求
        void queueBind(const std::string &ename, const std::string &qname, const std::string &key);

        // 向服务端发送解除队列绑定请求
        void queueUnBind(const std::string &ename, const std::string &qname);

        // 向服务端发送消息发布请求
        void basicPublish(const std::string &ename, BasicProperties *bp, const std::string &body);

        // 向服务端发送消息确认请求
        void basicAck(const std::string &msgid);

        // 向服务端发送订阅队列请求
        void basicConsume(const std::string &consumer_tag, const std::string &queue_name,
                          bool auto_ack, const ConsumerCallback &cb);

        // 取消当前 Channel 中消费者的队列订阅
        void basicCancel();

    private:
        std::string _cid;                   // 信道channel ID
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