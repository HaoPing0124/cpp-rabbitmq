#ifndef __M_HOST_H__
#define __M_HOST_H__

#include "mq_exchange.hpp"
#include "mq_queue.hpp"
#include "mq_binding.hpp"
#include "mq_message.hpp"

namespace haoping
{
    class VirtualHost
    {
    public:
        VirtualHost(const std::string &basedir, const std::string &dbfile);

        // 声明交换机
        bool declareExchange(const std::string &name,
                             ExchangeType type, bool durable, bool auto_delete,
                             std::unordered_map<std::string, std::string> &args);
        // 删除交换机
        void deleteExchange(const std::string &name);

        // 声明队列
        bool declareQueue(const std::string &qname,
                          bool qdurable,
                          bool qexclusive,
                          bool qauto_delete,
                          const std::unordered_map<std::string, std::string> &qargs);

        // 删除队列
        void deleteQueue(const std::string &name);

        // 增加绑定信息
        bool bind(const std::string &ename, const std::string &qname, const std::string &key, bool durable);
        // 解除绑定信息
        bool unBind(const std::string &ename, const std::string &qname);

        // 发布消息
        bool basicPublish(const std::string &qname, BasicProperties *bp, const std::string &body);

        // 推送消息(消费)
        MessagePtr basicConsume(const std::string &qname);

        // 确认请求
        void basicAck(const std::string &qname, const std::string &msgid);

        // 清理数据
        void clear();

    private:
        std::string _host_name;
        ExchangeManager::ptr _emp;
        MsgQueueManager::ptr _mqmp;
        BindingManager::ptr _bmp;
        MessageManager::ptr _mmp;
    };
}

#endif