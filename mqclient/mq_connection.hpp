#ifndef __M_CLIENT_CONNECTION_H__
#define __M_CLIENT_CONNECTION_H__
#include "muduo/proto/dispatcher.h"
#include "muduo/proto/codec.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/TcpClient.h"
#include "muduo/base/CountDownLatch.h"

#include "mq_channel.hpp"
#include "mq_worker.hpp"

namespace haoping
{
    class Connection
    {

    public:
        // 构造客户端连接对象
        // sip 表示服务端 IP 地址
        // sport 表示服务端监听端口
        // worker 表示客户端异步工作模块
        // worker 中的 EventLoopThread 负责运行客户端网络事件循环
        Connection(const std::string sip, int sport, const AsyncWorker::ptr &worker)
            : _latch(1),    // 将 CountDownLatch 的初始计数设置为 1, connect() 会等待计数减为 0 后再继续执行
              _client(worker->loopthread.startLoop(), muduo::net::InetAddress(sip, sport), "Client"), // 启动 worker 中的 EventLoopThread, 将返回的 EventLoop 交给 TcpClient 使用
              _dispatcher(std::bind(&Connection::onUnknownMessage, this, std::placeholders::_1,
                                    std::placeholders::_2, std::placeholders::_3)), // 当收到没有注册处理函数的 Protobuf 消息时 调用 Connection::onUnknownMessage() 进行处理
              _codec(std::make_shared<ProtobufCodec>(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher,
                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))), // codec 完成消息解码后，将消息交给 dispatcher 继续分发
              _worker(worker),
              _channel_manager(std::make_shared<ChannelManager>())
        {
            // 注册普通请求响应的处理函数
            // 当客户端收到 basicCommonResponse 时 dispatcher 会调用 Connection::basicResponse()
            _dispatcher.registerMessageCallback<basicCommonResponse>(std::bind(&Connection::basicResponse, this,
                                                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 注册消费消息推送的处理函数
            // 当客户端收到 basicConsumeResponse 时 dispatcher 会调用 Connection::consumerResponse()
            _dispatcher.registerMessageCallback<basicConsumeResponse>(std::bind(&Connection::consumerResponse, this,
                                                                                std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 设置 TCP 连接状态回调函数
            // 当连接建立或者连接断开时 TcpClient 会调用 Connection::onConnection()
            _client.setConnectionCallback(std::bind(&Connection::onConnection, this, std::placeholders::_1));

            // 设置 TCP 消息回调函数
            // 当 TcpClient 收到网络数据时 先调用 ProtobufCodec::onMessage() 完成拆包和解码
            _client.setMessageCallback(std::bind(&ProtobufCodec::onMessage, _codec,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

            // 向服务端发起异步 TCP 连接
            // 该函数调用后，真正的连接建立过程由 EventLoop 线程完成
            _client.connect();

            // 当前线程在此处阻塞等待 直到连接建立成功
            // 当 onConnection() 确认连接建立成功后 会调用 _latch.countDown() 将计数减为 0
            _latch.wait();
        }

    public:
        Channel::ptr openChannel()
        {
            Channel::ptr channel = _channel_manager->create(_conn, _codec);
            if (channel.get() == nullptr)
            {
                DLOG("创建信道对象失败！");
                return Channel::ptr();
            }

            bool ret = channel->openChannel();
            if (ret == false)
            {
                DLOG("打开信道失败！");
                return Channel::ptr();
            }
            return channel;
        }

        void closeChannel(const Channel::ptr &channel)
        {
            if (channel.get() == nullptr)
            {
                DLOG("关闭信道失败，信道对象为空！");
                return;
            }
            
            channel->closeChannel();
            _channel_manager->remove(channel->cid());
        }

    private:
        // 处理服务端返回的普通请求响应
        void basicResponse(const muduo::net::TcpConnectionPtr &conn,
                           const basicCommonResponsePtr &message, muduo::Timestamp);

        // 处理服务端主动推送的消费消息
        void consumerResponse(const muduo::net::TcpConnectionPtr &conn,
                              const basicConsumeResponsePtr &message, muduo::Timestamp);

        // 处理没有注册对应处理函数的 Protobuf 消息
        void onUnknownMessage(const muduo::net::TcpConnectionPtr &conn,
                              const MessagePtr &message, muduo::Timestamp)
        {
            LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
            conn->shutdown(); // 主动关闭当前 TCP 连接
        }

        // 处理 TCP 连接建立和断开事件
        void onConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            if (conn->connected())
            {
                _conn = conn;

                // 将 CountDownLatch 的计数减为 0
                // 唤醒正在 connect() 中等待的线程
                _latch.countDown();
            }
            else
            {
                // 连接关闭
                // 释放客户端保存的 TCP 连接智能指针
                _conn.reset();
            }
        }

    private:
        muduo::CountDownLatch _latch;         // 同步计数器
        muduo::net::TcpConnectionPtr _conn;   // 当前客户端与服务端之间的 TCP 连接
        muduo::net::TcpClient _client;        // muduo 客户端对象
        ProtobufDispatcher _dispatcher;       // Protobuf 消息分发器
        ProtobufCodecPtr _codec;              // Protobuf 协议编解码器
        AsyncWorker::ptr _worker;             // 客户端异步工作模块
        ChannelManager::ptr _channel_manager; // 客户端信道管理器
    };
}

#endif