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
            // 获取到所有的队列信息，通过队列名称恢复历史消息数据
            QueueMap qm = _mqmp->allQueues();
            for (auto &q : qm)
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
            // 删除交换机的时候，需要将交换机相关的绑定信息也删除掉。
            _bmp->removeExchangeBindings(name);
            return _emp->deleteExchange(name);
        }

        // 声明队列
        bool declareQueue(const std::string &qname,
                          bool qdurable,
                          bool qexclusive,
                          bool qauto_delete,
                          const std::unordered_map<std::string, std::string> &qargs)
        {
            // 初始化队列的消息句柄（消息的存储管理）
            // 队列的创建
            _mmp->initQueueMessage(qname);
            return _mqmp->declareQueue(qname, qdurable, qexclusive, qauto_delete, qargs);
        }

        // 删除队列
        void deleteQueue(const std::string &name)
        {
            // 删除的时候队列相关的数据有两个：队列的消息，队列的绑定信息
            _mmp->destroyQueueMessage(name);
            _bmp->removeQueueBindings(name);
            return _mqmp->deleteQueue(name);
        }

        // 增加绑定信息
        bool bind(const std::string &ename, const std::string &qname, const std::string &key, bool durable)
        {
            // 检查是否有此交换机
            Exchange::ptr ep = _emp->selectExchange(ename);
            if (ep.get() == nullptr)
            {
                DLOG("进行队列绑定失败，交换机%s不存在！", ename.c_str());
                return false;
            }

            // 检查是否有此队列
            MsgQueue::ptr mqp = _mqmp->selectQueue(qname);
            if (mqp.get() == nullptr)
            {
                DLOG("进行队列绑定失败，队列%s不存在！", qname.c_str());
                return false;
            }
            // 交换机 和 队列同时持久化才能设置为持久化
            return _bmp->bind(ename, qname, key, ep->durable && mqp->durable);
        }

        // 解除绑定信息
        bool unBind(const std::string &ename, const std::string &qname)
        {
            return _bmp->unBind(ename, qname);
        }

        // 获取交换机绑定信息
        MsgQueueBindingMap exchangeBindings(const std::string &ename)
        {
            return _bmp->getExchangeBindings(ename);
        }

        // 发布消息
        bool basicPublish(const std::string &qname, BasicProperties *bp, const std::string &body)
        {
            // 检查是否有此队列
            MsgQueue::ptr mqp = _mqmp->selectQueue(qname);
            if (mqp.get() == nullptr)
            {
                DLOG("发布消息失败，队列%s不存在！", qname.c_str());
                return false;
            }
            return _mmp->insert(qname, bp, body, mqp->durable);
        }

        // 推送消息(消费)
        MessagePtr basicConsume(const std::string &qname)
        {
            return _mmp->front(qname);
        }

        // 确认请求
        void basicAck(const std::string &qname, const std::string &msgid)
        {
            return _mmp->ack(qname, msgid);
        }

        // 清理数据
        void clear();

    private:
        std::string _host_name;
        ExchangeManager::ptr _emp;  // 交换机操作句柄
        MsgQueueManager::ptr _mqmp; // 队列操作句柄
        BindingManager::ptr _bmp;   // 绑定信息操作句柄
        MessageManager::ptr _mmp;   // 消息管理操作句柄
    };
}

#endif