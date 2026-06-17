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
        {
        }

        // 创建消息文件
        bool createMsgFile()
        {
            if (FileHelper(_datafile).exists() == true)
            {
                return true;
            }

            bool ret = FileHelper::createFile(_datafile);
            if (ret == false)
            {
                DLOG("创建队列数据文件 %s 失败！", _datafile.c_str());
                return false;
            }
            return true;
        }

        // 删除消息文件
        void removeMsgFile()
        {
            // 只删除文件不删除目录
            FileHelper::removeFile(_datafile);
            FileHelper::removeFile(_tmpfile);
        }

        // 新增消息的持久化
        bool insert(const std::string &filename, MessagePtr &msg)
        {
            // 新增数据都是添加在文件末尾的
            // 1. 进行消息的序列化，获取到格式化后的消息
            std::string body = msg->payload().SerializeAsString();

            // 2. 获取文件长度
            FileHelper helper(filename);
            size_t fsize = helper.size();
            size_t msg_size = body.size();

            // 写入逻辑：1. 先写入4字节数据长度， 2， 再写入指定长度数据
            bool ret = helper.write((char *)&msg_size, fsize, sizeof(size_t));
            if (ret == false)
            {
                DLOG("向队列数据文件写入数据长度失败！");
                return false;
            }

            // 3. 将数据写入文件的指定位置
            ret = helper.write(body.c_str(), fsize + sizeof(size_t), body.size());
            if (ret == false)
            {
                DLOG("向队列数据文件写入数据失败！");
                return false;
            }

            // 4. 更新msg中的实际存储信息
            msg->set_offset(fsize + sizeof(size_t));
            msg->set_length(body.size());
            return true;
        }

        // 删除消息的持久化
        bool remove(const MessagePtr &msg)
        {
            // 1. 将msg中的有效标志位修改为 '0'
            msg->mutable_payload()->set_valid("0");

            // 2. 对msg进行序列化
            std::string body = msg->payload().SerializeAsString();
            if (body.size() != msg->length())
            {
                DLOG("不能修改文件中的数据信息，因为新生成的数据与原数据长度不一致!");
                return false;
            }

            // 3. 将序列化后的消息，写入到数据在文件中的指定位置（覆盖原有的数据）
            FileHelper helper(_datafile);
            bool ret = helper.write(body.c_str(), msg->offset(), body.size());
            if (ret == false)
            {
                DLOG("向队列数据文件写入数据失败！");
                return false;
            }

            // DLOG("确认消息后，删除持久化消息成功：%s", msg->payload().body().c_str());
            return true;
        }

        // 垃圾回收
        std::list<MessagePtr> gc();

    private:
        std::string _qname;    // 队列名
        std::string _datafile; // 消息数据文件
        std::string _tmpfile;  // 临时文件
    };
}

#endif