#ifndef __M_MESSAGE_H__
#define __M_MESSAGE_H__

// 交换机数据管理模块
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <memory>


namespace haoping
{
#define DATAFILE_SUBFIX ".mqd";
#define TMPFILE_SUBFIX ".mqd.tmp";
    using MessagePtr = std::shared_ptr<haoping::Message>;
    class MessageMapper
    {
    public:
        MessageMapper(const std::string &basedir, const std::string &qname)
        {}

        // 创建消息文件
        void createMsgFile();
        
        // 删除消息文件
        void removeMsgFile();

        // 新增消息的持久化
        void insert(MessagePtr &msg);

        // 删除消息的持久化
        void remove(const MessagePtr &msg);

        // 垃圾回收
        std::list<MessagePtr> gc();
    private:
        std::string _qname;     // 队列名
        std::string _datafile;  // 原文件
        std::string _tmpfile;   // 临时文件
    };
}

#endif