#include "muduo/proto/dispatcher.h"
#include "muduo/proto/codec.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/TcpClient.h"
#include "muduo/base/CountDownLatch.h"

#include "request.pb.h"
#include <iostream>

class Client
{
    typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
    typedef std::shared_ptr<haoping::TranslateResponse> TranslateResponsePtr;
    typedef std::shared_ptr<haoping::AddResponse> AddResponsePtr;

public:
    Client(const std::string sip, int sport)
        : _latch(1), _client(_loopthread.startLoop(), muduo::net::InetAddress(sip, sport), "Client"),
          _dispatcher(std::bind(&Client::onUnknownMessage, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3)),
          _codec(std::bind(&ProtobufDispatcher::onProtobufMessage, &_dispatcher,
                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
    {
        _dispatcher.registerMessageCallback<haoping::TranslateResponse>(std::bind(&Client::onTranslate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        _dispatcher.registerMessageCallback<haoping::AddResponse>(std::bind(&Client::onAdd, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        _client.setConnectionCallback(std::bind(&Client::onConnection, this, std::placeholders::_1));
        _client.setMessageCallback(std::bind(&ProtobufCodec::onMessage, &_codec,
                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    }

    // 连接服务器——需要阻塞等待连接建立成功后返回
    void connect()
    {
        _client.connect();
        _latch.wait(); // 阻塞等待，直到连接建立成功
    }

    void Translate(const std::string &msg)
    {
        haoping::TranslateRequest req;
        req.set_msg(msg);
        send(&req);
    }

    void Add(int num1, int num2)
    {
        haoping::AddRequest req;
        req.set_num1(num1);
        req.set_num2(num2);
        send(&req);
    }

private:
    bool send(const google::protobuf::Message *message)
    {
        if (_conn->connected()) // 连接状态正常再发送，否则返回false
        {
            _codec.send(_conn, *message);
            return true;
        }
        return false;
    }

    void onUnknownMessage(const muduo::net::TcpConnectionPtr &conn,
                          const MessagePtr &message, muduo::Timestamp)
    {
        LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
        conn->shutdown();
    }

    void onTranslate(const muduo::net::TcpConnectionPtr &conn,
                     const TranslateResponsePtr &message, muduo::Timestamp)
    {
        std::cout << "翻译结果：" << message->msg() << std::endl;
    }

    void onAdd(const muduo::net::TcpConnectionPtr &conn,
               const AddResponsePtr &message, muduo::Timestamp)
    {
        std::cout << "加法结果：" << message->result() << std::endl;
    }

    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if(conn->connected())
        {
            _latch.countDown(); // 唤醒主线程的阻塞    
            _conn = conn;
        }
        else
        {
            // 连接关闭
            _conn.reset();
        }
    }

private:
    muduo::CountDownLatch _latch;            // 实现同步
    muduo::net::EventLoopThread _loopthread; // 异步循环处理线程
    muduo::net::TcpConnectionPtr _conn;      // 客户端对应连接
    muduo::net::TcpClient _client;           // 客户端
    ProtobufDispatcher _dispatcher;          // 请求分发器
    ProtobufCodec _codec;                    // 协议处理器
};

int main()
{
    Client client("127.0.0.1", 8085);
    client.connect();

    client.Translate("apple");
    client.Add(11, 22);

    sleep(1);

    return 0;
}