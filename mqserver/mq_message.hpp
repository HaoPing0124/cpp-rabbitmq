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
        MessageMapper(std::string &basedir, const std::string &qname)
            : _qname(qname)
        {
            // 如果目录不是以 / 结尾，就补一个 /，方便后面拼路径
            if (basedir.back() != '/')
                basedir.push_back('/');

            // 拼出主数据文件路径
            _datafile = basedir + qname + DATAFILE_SUBFIX;

            // 拼出临时文件路径
            _tmpfile = basedir + qname + TMPFILE_SUBFIX;

            // 如果目录不存在，就创建目录
            if (FileHelper(basedir).exists() == false)
            {
                assert(FileHelper::createDirectory(basedir));
            }

            // 确保主数据文件存在
            createMsgFile();
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
            insert(_datafile, msg);
        }

        // 删除消息的持久化
        bool remove(MessagePtr &msg)
        {
            // 1. 将msg中的有效标志位修改为 '0', 表示这条消息无效了
            msg->mutable_payload()->set_valid("0");

            // 2. 把修改后的 payload 序列化成字符串
            std::string body = msg->payload().SerializeAsString();
            // 新数据和旧数据长度必须相同
            if (body.size() != msg->length())
            {
                DLOG("不能修改文件中的数据信息，因为新生成的数据与原数据长度不一致!");
                return false;
            }

            // 3. 将序列化后的消息("0")，写入到数据在文件中的指定位置(偏移量位置)(覆盖原有的数据)
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
        std::list<MessagePtr> gc()
        {
            bool ret;
            std::list<MessagePtr> result;

            // 1. 先加载出当前文件里的有效消息
            ret = load(result);
            if (ret == false)
            {
                DLOG("加载有效数据失败！");
                return result;
            }

            // 2. 创建临时文件，把有效消息重新写进去
            FileHelper::createFile(_tmpfile);
            for (auto &msg : result)
            {
                DLOG("向临时文件写入数据: %s", msg->payload().body().c_str());
                ret = insert(_tmpfile, msg);
                if (ret == false)
                {
                    DLOG("向临时文件写入消息数据失败！！");
                    return result;
                }
            }

            // 3. 删除旧数据文件
            ret = FileHelper::removeFile(_datafile);
            if (ret == false)
            {
                DLOG("删除源文件失败！");
                return result;
            }

            // 4. 把临时文件改名成正式数据文件
            ret = FileHelper(_tmpfile).rename(_datafile);
            if (ret == false)
            {
                DLOG("修改临时文件名称失败！");
                return result;
            }

            // 5. 返回新的有效消息列表
            return result;
        }

    private:
        bool load(std::list<MessagePtr> &result)
        {
            // 1. 打开主数据文件，按“长度 + 数据”的格式逐条读取
            FileHelper data_file_helper(_datafile);
            size_t offset = 0, msg_size;
            size_t fsize = data_file_helper.size();
            bool ret;

            while (offset < fsize)
            {
                // 2. 先读取消息长度
                ret = data_file_helper.read((char *)&msg_size, offset, sizeof(size_t));
                if (ret == false)
                {
                    DLOG("读取消息长度失败！");
                    return false;
                }
                offset += sizeof(size_t);

                // 3. 根据消息长度，读取消息内容
                std::string msg_body(msg_size, '\0');
                ret = data_file_helper.read(&msg_body[0], offset, msg_size);
                if (ret == false)
                {
                    DLOG("读取消息数据失败！");
                    return false;
                }
                offset += msg_size;

                // 4. 构造 Message 对象，并解析 payload
                MessagePtr msgp = std::make_shared<Message>();
                msgp->mutable_payload()->ParseFromString(msg_body);

                // 5. 如果消息已被标记为无效，就跳过
                if (msgp->payload().valid() == "0")
                {
                    DLOG("加载到无效消息：%s", msgp->payload().body().c_str());
                    continue;
                }

                // 6. 有效消息保存起来
                result.push_back(msgp);
            }
            return true;
        }

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

    private:
        std::string _qname;    // 队列名
        std::string _datafile; // 消息数据文件
        std::string _tmpfile;  // 临时文件
    };

    class QueueMessage
    {
    public:
        using ptr = std::shared_ptr<QueueMessage>;
        QueueMessage(std::string &basedir, const std::string &qname) : _mapper(basedir, qname),
                                                                       _qname(qname), _valid_count(0), _total_count(0) {}
        bool recovery()
        {
            // 恢复历史消息
            std::unique_lock<std::mutex> lock(_mutex);
            _msgs = _mapper.gc();
            for (auto &msg : _msgs)
            {
                _durable_msgs.insert(std::make_pair(msg->payload().properties().id(), msg));
            }
            _valid_count = _total_count = _msgs.size();
            return true;
        }

        bool insert(const BasicProperties *bp, const std::string &body, bool queue_is_durable);
        MessagePtr front();

        // 每次删除消息后，判断是否需要垃圾回收
        bool remove(const std::string &msg_id);
        size_t getable_count();
        size_t total_count();
        size_t durable_count();
        size_t waitack_count();
        void clear();

    private:
        std::mutex _mutex;
        std::string _qname;
        size_t _valid_count;
        size_t _total_count;
        MessageMapper _mapper;
        std::list<MessagePtr> _msgs;                               // 待推送消息
        std::unordered_map<std::string, MessagePtr> _durable_msgs; // 持久化消息hash
        std::unordered_map<std::string, MessagePtr> _waitack_msgs; // 待确认消息hash
    };

}

#endif