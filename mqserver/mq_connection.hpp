#ifndef __H_CONNECTION_H__
#define __H_CONNECTION_H__

#include "mq_channel.hpp"

namespace haoping
{
    // Connection表示服务端与一个客户端之间的连接管理对象
    // 一个Connection对应一个客户端TCP连接
    // Connection内部可以创建和管理多个Channel信道
    class Connection
    {
    public:
        using ptr = std::shared_ptr<Connection>;

        // 客户端连接管理对象
        // host 表示虚拟主机，用于管理交换机、队列、绑定关系和消息
        // cmp 表示消费者管理器，用于创建、选择和删除消费者
        // codec 表示Protobuf编解码器，用于向客户端发送响应
        // conn 表示当前客户端对应的TCP连接
        // pool 表示线程池，用于异步执行消息消费任务
        Connection(const VirtualHost::ptr &host,
                   const ConsumerManager::ptr &cmp,
                   const ProtobufCodecPtr &codec,
                   const muduo::net::TcpConnectionPtr &conn,
                   const ThreadPool::ptr &pool) : _conn(conn),
                                                  _codec(codec),
                                                  _cmp(cmp),
                                                  _host(host),
                                                  _pool(pool),
                                                  _channels(std::make_shared<ChannelManager>()) {}

        // 处理客户端发送的打开信道请求
        void openChannel(const openChannelRequestPtr &req)
        {
            // 1. 判断信道ID是否重复,创建信道
            bool ret = _channels->openChannel(req->cid(), _host, _cmp, _codec, _conn, _pool);
            if (ret == false)
            {
                DLOG("创建信道的时候，信道ID重复了");
                return basicResponse(false, req->rid(), req->cid());
            }
            DLOG("%s 信道创建成功！", req->cid().c_str());

            // 2. 给客户端进行回复
            return basicResponse(true, req->rid(), req->cid());
        }

        // 处理客户端发送的关闭信道请求
        void closeChannel(const closeChannelRequestPtr &req)
        {
            _channels->closeChannel(req->cid());
            return basicResponse(true, req->rid(), req->cid());
        }

        // 根据信道ID获取当前连接中的Channel对象
        // 后续的交换机、队列和消息请求都需要先找到对应信道
        Channel::ptr getChannel(const std::string &cid)
        {
            return _channels->getChannel(cid);
        }

    private:
        void basicResponse(bool ok, const std::string &rid, const std::string &cid)
        {
            basicCommonResponse resp;
            resp.set_rid(rid);
            resp.set_cid(cid);
            resp.set_ok(ok);
            _codec->send(_conn, resp);
        }

    private:
        muduo::net::TcpConnectionPtr _conn; // 当前Connection对应的客户端TCP连接
        ProtobufCodecPtr _codec;            // Protobuf编解码器
        ConsumerManager::ptr _cmp;          // 消费者管理器
        VirtualHost::ptr _host;             // 虚拟主机
        ThreadPool::ptr _pool;              // 异步任务线程池
        ChannelManager::ptr _channels;      // 当前客户端连接对应的信道管理器
    };

    // 连接管理器
    // 负责创建、保存、查找和删除服务器中的Connection对象
    class ConnectionManager
    {
    public:
        using ptr = std::shared_ptr<ConnectionManager>;
        ConnectionManager() {}

        // 创建并保存一个新的客户端连接管理对象
        // host 表示虚拟主机，用于管理交换机、队列、绑定关系和消息
        // cmp 表示消费者管理器，用于管理队列对应的消费者
        // codec 表示 Protobuf 编解码器，用于向客户端发送响应
        // conn 表示客户端与服务端之间的 TCP 连接
        // pool 表示线程池，用于异步执行消息消费任务
        void newConnection(const VirtualHost::ptr &host,
                           const ConsumerManager::ptr &cmp,
                           const ProtobufCodecPtr &codec,
                           const muduo::net::TcpConnectionPtr &conn,
                           const ThreadPool::ptr &pool)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conns.find(conn);
            if (it != _conns.end())
            {
                return;
            }

            // 为当前 TCP 连接创建一个 Connection 对象
            // Connection 对象负责管理该客户端连接内部的所有 Channel
            Connection::ptr self_conn = std::make_shared<Connection>(host, cmp, codec, conn, pool);
            _conns.insert(std::make_pair(conn, self_conn));
        }

        // 删除指定客户端 TCP 连接对应的 Connection 对象
        void delConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _conns.erase(conn);
        }

        // 根据客户端 TCP 连接获取对应的 Connection 对象
        Connection::ptr getConnection(const muduo::net::TcpConnectionPtr &conn)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _conns.find(conn);
            if (it == _conns.end())
            {
                return Connection::ptr();
            }
            return it->second;
        }

    private:
        std::mutex _mutex;
        // 保存 TCP 连接与 Connection 对象之间的对应关系
        // key 表示 muduo 中的客户端 TCP 连接
        // value 表示项目中对该客户端连接进行业务管理的 Connection 对象
        std::unordered_map<muduo::net::TcpConnectionPtr, Connection::ptr> _conns;
    };
}

#endif