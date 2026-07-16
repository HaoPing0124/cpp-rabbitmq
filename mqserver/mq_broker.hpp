#ifndef __M_BROKER_H__
#define __M_BROKER_H__
#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"
#include "../mqcommon/mq_threadpool.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_proto.pb.h"
#include "../mqcommon/mq_logger.hpp"
#include "mq_connection.hpp"
#include "mq_consumer.hpp"
#include "mq_host.hpp"

namespace haoping
{
    class Server
    {
    public:
        typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
        Server(int port, const std::string &basedir) : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
                                                               "Server", muduo::net::TcpServer::kReusePort),
                                                       _dispatcher(std::bind(&Server::onUnknownMessage, this, std::placeholders::_1,
                                                                             std::placeholders::_2, std::placeholders::_3)),
                                                       _codec(std::make_shared<ProtobufCodec>(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher,
                                                                                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)))
        {
            // 注册业务请求处理函数
            _dispatcher.registerMessageCallback<haoping::openChannelRequest>(std::bind(&Server::onOpenChannel, this,
                                                                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::closeChannelRequest>(std::bind(&Server::onCloseChannel, this,
                                                                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::declareExchangeRequest>(std::bind(&Server::onDeclareExchange, this,
                                                                                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::deleteExchangeRequest>(std::bind(&Server::onDeleteExchange, this,
                                                                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::declareQueueRequest>(std::bind(&Server::onDeclareQueue, this,
                                                                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::deleteQueueRequest>(std::bind(&Server::onDeleteQueue, this,
                                                                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::queueBindRequest>(std::bind(&Server::onQueueBind, this,
                                                                                     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::queueUnBindRequest>(std::bind(&Server::onQueueUnBind, this,
                                                                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::basicPublishRequest>(std::bind(&Server::onBasicPublish, this,
                                                                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::basicAckRequest>(std::bind(&Server::onBasicAck, this,
                                                                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::basicConsumeRequest>(std::bind(&Server::onBasicConsume, this,
                                                                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _dispatcher.registerMessageCallback<haoping::basicCancelRequest>(std::bind(&Server::onBasicCancel, this,
                                                                                       std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            _server.setMessageCallback(std::bind(&ProtobufCodec::onMessage, _codec.get(),
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
            _server.setConnectionCallback(std::bind(&Server::onConnection, this, std::placeholders::_1));
        }

        void start()
        {
            _server.start();
            _baseloop.loop();
        }

    private:
        // 打开信道
        void onOpenChannel(const muduo::net::TcpConnectionPtr &conn, const openChannelRequestPtr &message, muduo::Timestamp);

        // 关闭信道
        void onCloseChannel(const muduo::net::TcpConnectionPtr &conn, const closeChannelRequestPtr &message, muduo::Timestamp);

        // 声明交换机
        void onDeclareExchange(const muduo::net::TcpConnectionPtr &conn, const declareExchangeRequestPtr &message, muduo::Timestamp);

        // 删除交换机
        void onDeleteExchange(const muduo::net::TcpConnectionPtr &conn, const deleteExchangeRequestPtr &message, muduo::Timestamp);

        // 声明队列
        void onDeclareQueue(const muduo::net::TcpConnectionPtr &conn, const declareQueueRequestPtr &message, muduo::Timestamp);

        // 删除队列
        void onDeleteQueue(const muduo::net::TcpConnectionPtr &conn, const deleteQueueRequestPtr &message, muduo::Timestamp);

        // 队列绑定
        void onQueueBind(const muduo::net::TcpConnectionPtr &conn, const queueBindRequestPtr &message, muduo::Timestamp);

        // 队列解绑
        void onQueueUnBind(const muduo::net::TcpConnectionPtr &conn, const queueUnBindRequestPtr &message, muduo::Timestamp);

        // 消息发布
        void onBasicPublish(const muduo::net::TcpConnectionPtr &conn, const basicPublishRequestPtr &message, muduo::Timestamp);

        // 消息确认
        void onBasicAck(const muduo::net::TcpConnectionPtr &conn, const basicAckRequestPtr &message, muduo::Timestamp);

        // 队列消息订阅
        void onBasicConsume(const muduo::net::TcpConnectionPtr &conn, const basicConsumeRequestPtr &message, muduo::Timestamp);

        // 队列消息取消订阅
        void onBasicCancel(const muduo::net::TcpConnectionPtr &conn, const basicCancelRequestPtr &message, muduo::Timestamp);

        void onUnknownMessage(const muduo::net::TcpConnectionPtr &conn, const MessagePtr &message, muduo::Timestamp);

        void onConnection(const muduo::net::TcpConnectionPtr &conn);

    private:
        muduo::net::EventLoop _baseloop;
        muduo::net::TcpServer _server;  // 服务器对象
        ProtobufDispatcher _dispatcher; // 请求分发器对象--要向其中注册请求处理函数
        ProtobufCodecPtr _codec;        // protobuf协议处理器--针对收到的请求数据进行protobuf协议处理
        VirtualHost::ptr _virtual_host;
        ConsumerManager::ptr _consumer_manager;
        ConnectionManager::ptr _connection_manager;
        ThreadPool::ptr _threadpool;
    };
}
#endif