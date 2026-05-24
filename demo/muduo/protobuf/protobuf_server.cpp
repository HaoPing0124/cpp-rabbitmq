#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"

#include "request.pb.h"
#include <iostream>

class Server
{
    typedef std::shared_ptr<haoping::TranslateRequest> TranslateRequestPtr;
    typedef std::shared_ptr<haoping::TranslateResponse> TranslateResponsePtr;
    typedef std::shared_ptr<haoping::AddRequest> AddRequestPtr;
    typedef std::shared_ptr<haoping::AddResponse> AddResponsePtr;

public:
    Server(int port) : _server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
                               "Server", muduo::net::TcpServer::kReusePort),
                       _dispatcher(std::bind(&Server::onUnknownMessage, this, std::placeholders::_1,
                                             std::placeholders::_2, std::placeholders::_3)),
                       _codec(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher,
                                        std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    {
        // 注册用户请求处理函数
        _dispatcher.registerMessageCallback<haoping::TranslateRequest>
            (std::bind(&Server::onTranslate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        _dispatcher.registerMessageCallback<haoping::AddRequest>
            (std::bind(&Server::onAdd, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        _server.setConnectionCallback(std::bind(&Server::onConnection, this, std::placeholders::_1));

        _server.setMessageCallback(std::bind(&ProtobufCodec::onMessage, &_codec,
            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    void start()
    {
        _server.start();
        _baseloop.loop();
    }

private:
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

    void onTranslate(const muduo::net::TcpConnectionPtr &conn,
                     const TranslateRequestPtr &message, muduo::Timestamp)
    {
        // 1.提取message有效信息（需要翻译内容）
        std::string req_msg = message->msg();

        // 2.进行翻译 得到结果
        std::string rsp_msg = translate(req_msg);

        // 3.组织 Protobuf 响应
        haoping::TranslateResponse resp;
        resp.set_msg(rsp_msg);

        // 4.发送响应
        _codec.send(conn, resp);
    }

    void onAdd(const muduo::net::TcpConnectionPtr &conn,
               const AddRequestPtr &message, muduo::Timestamp)
    {
        int num1 = message->num1();
        int num2 = message->num2();
        int result = num1 + num2;
        
        haoping::AddResponse resp;
        resp.set_result(result);
        _codec.send(conn, resp);
    }

    void onUnknownMessage(const muduo::net::TcpConnectionPtr &conn,
                          const MessagePtr &message, muduo::Timestamp)
    {
        LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
        conn->shutdown();
    }

    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if(conn->connected()){
            LOG_INFO << "新连接建立成功！";
        }else {
            LOG_INFO << "连接即将关闭！";
        }
    }

private:
    muduo::net::EventLoop _baseloop;
    muduo::net::TcpServer _server;  // 服务器对象
    ProtobufDispatcher _dispatcher; // 请求分发器对象——要向其中注册请求处理函数
    ProtobufCodec _codec;           // protobuf协议处理器——针对收到的请求数据进行Protobuf协议处理
};

int main()
{
    Server server(8085);
    server
    return 0;
}