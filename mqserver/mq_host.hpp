#ifndef __M_HOST_H__
#define __M_HOST_H__

#include "mq_exchange.hpp"
#include "mq_queue.hpp"
#include "mq_binding.hpp"
#include "mq_message.hpp"

namespace haoping
{
    // 虚拟主机管理类
    class VirtualHost
    {
    public:
        VirtualHost(const std::string const host_name, std::string &basedir, const std::string &dbfile)
            : _host_name(host_name),
              _emp(std::make_shared<ExchangeManager>(dbfile)),
              _mqmp(std::make_shared<MsgQueueManager>(dbfile)),
              _bmp(std::make_shared<BindingManager>(dbfile)),
              _mmp(std::make_shared<MessageManager>(basedir))
        {
            //获取到所有的队列信息，通过队列名称恢复历史消息数据
            QueueMap qm = _mqmp->allQueues();
            for(auto &q : qm)
            {
                _mmp->initQueueMessage(q.first);
            }
            
        }

        // 声明交换机
        bool declareExchange(const std::string &name,
                             ExchangeType type, bool durable, bool auto_delete,
                             std::unordered_map<std::string, std::string> &args)
        {
            return _emp->declareExchange(name, type, durable, auto_delete, args);
        }

        // 删除交换机
        void deleteExchange(const std::string &name)
        {
            //删除交换机的时候，需要将交换机相关的绑定信息也删除掉。
            _bmp->removeExchangeBindings(name);
            return _emp->deleteExchange(name);
        }

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