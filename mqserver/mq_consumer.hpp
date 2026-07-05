#ifndef __M_CONSUMER_H__
#define __M_CONSUMER_H__
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <vector>
#include <functional>

namespace bitmq
{
    struct Consumer
    {
        using ptr = std::shared_ptr<Consumer>;
        std::string tag;   // 消费者标识
        std::string qname; // 消费者订阅的队列名称
        bool auto_ack;     // 自动确认标志

        Consumer()
        {
            DLOG("new Consumer: %p", this);
        }

        ~Consumer()
        {
            DLOG("del Consumer: %p", this);
        }
    };

    // 以队列为单元的消费者管理结构
    class QueueConsumer
    {
    public:
        using ptr = std::shared_ptr<QueueConsumer>;
        QueueConsumer(const std::string &qname) : _qname(qname), _rr_seq(0) {}

        // 队列新增消费者
        Consumer::ptr create();

        // 队列移除消费者
        void remove();

        // 队列获取消费者：RR轮转获取
        Consumer::ptr choose();

        // 是否为空
        bool empty();
        
        // 判断指定消费者是否存在
        bool exists();
        
        // 清理所有消费者
        void clear();

    private:
        std::string _qname;
        std::mutex _mutex;
        uint64_t _rr_seq; // 轮转序号
        std::vector<Consumer::ptr> _consumers;
    };
}

#endif