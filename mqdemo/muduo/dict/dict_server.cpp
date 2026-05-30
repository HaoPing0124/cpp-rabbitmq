#include <iostream>
#include <functional>
#include <unordered_map>
#include "include/muduo/net/TcpServer.h"
#include "include/muduo/net/EventLoop.h"
#include "include/muduo/net/TcpConnection.h"

class TranslateServer
{
public:
    TranslateServer(int port)
        // 参数：1.主反应堆, 2.监听地址和端口, 3.服务器名称, 4.配置选项(是否启用端口复用)
        : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
                  "TranslateServer", muduo::net::TcpServer::kReusePort) // 默认kNoReusePort(不开启)
    {
        // 将类成员函数，设置为服务器的回调处理函数
        // std::bind 函数适配器函数，对函数进行参数绑定(解决单参数传入this指针问题)
        _server.setConnectionCallback(std::bind(&TranslateServer::onConnection, this, std::placeholders::_1));
        _server.setMessageCallback(std::bind(&TranslateServer::onMessage, this,
                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    // 启动服务器
    void start()
    {
        _server.start();  // 开始事件监听
        _baseloop.loop(); // 开始事件监控 是一个死循环阻塞接口
    }

private:
    // 新连接建立成功时的回调函数
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if (conn->connected() == true)
        {
            std::cout << "新连接建立成功!\n";
        }
        else
        {
            std::cout << "新连接关闭!\n";
        }
    }

    std::string translate(const std::string &str)
    {
        std::unordered_map<std::string, std::string> dict_map = {
            {"hello", "你好"},
            {"你好", "hello"},
            {"apple", "苹果"},
            {"苹果", "apple"}};

        auto it = dict_map.find(str);
        if (it == dict_map.end())
        {
            return "翻译错误，没有对应词语";
        }
        return it->second;
    }
    // 通信连接收到请求时的回调函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buf, muduo::Timestamp)
    {
        // 1.从buf中把请求的数据取出来
        std::string str = buf->retrieveAllAsString();

        // 2.调用translate接口进行翻译
        std::string resp = translate(str);

        // 3.对客户端进行相应结果
        conn->send(resp);
    }

private:
    // epoll事件监控，会进行描述符的事件监控，触发事件后进行io处理
    muduo::net::EventLoop _baseloop;

    // 用于设置回调函数，告诉服务器收到什么请求该怎么处理
    muduo::net::TcpServer _server;
};

int main()
{
    TranslateServer server(8085);
    server.start();
    return 0;
}