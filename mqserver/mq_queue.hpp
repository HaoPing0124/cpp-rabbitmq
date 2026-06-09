#ifndef __M_QUEUE_H__
#define __M_QUEUE_H__

// 队列数据管理模块
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace haoping
{
    struct MsgQueue
    {
        using ptr = std::shared_ptr<MsgQueue>;
        std::string name;
        bool durable;
        bool exclusive;
        bool auto_delete;
        google::protobuf::Map<std::string, std::string> args;

        MsgQueue() {}
        MsgQueue(const std::string &qname,
                 bool qdurable,
                 bool qexclusive,
                 bool qauto_delete,
                 const google::protobuf::Map<std::string, std::string> &qargs)
            : name(qname), durable(qdurable), exclusive(qexclusive),
              auto_delete(qauto_delete), args(qargs)
        {
        }

        void setArgs(const std::string &str_args)
        {
            std::vector<std::string> sub_args;
            StrHelper::split(str_args, "&", sub_args);
            for (auto &str : sub_args)
            {
                size_t pos = str.find("=");
                std::string key = str.substr(0, pos);
                std::string val = str.substr(pos + 1);
                args[key] = val;
            }
        }
        
        std::string getArgs()
        {
            std::string result;
            for (auto start = args.begin(); start != args.end(); ++start)
            {
                result += start->first + "=" + start->second + "&";
            }
            return result;
        }
    };
}

#endif