#ifndef __M_BINDING_H__
#define __M_BINDING_H__

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
    // 绑定信息描述类
    struct Binding
    {
        using ptr = std::shared_ptr<Binding>;
        std::string exchange_name;  // 交换机名称
        std::string msgqueue_name;  // 队列名称
        std::string binding_key;    // 绑定信息

        Binding(){}
        Binding(const std::string &ename, const std::string &qname, const std::string &key)
            :exchange_name(ename), msgqueue_name(qname), binding_key(key)
        {}
    };

    // 队列与绑定信息是一一对应的(因为是给某个交换机去绑定队列，因此一个交换机可能有多个队列的绑定信息)
    // 因此先定义一个 队列名 与 绑定信息 的映射关系，这是为了方便通过队列名查找绑定信息
    // 队列与绑定信息的关系
    using MsgQueueBindingMap = std::unordered_map<std::string, Binding::ptr>;

    // 定义一个 交换机名称 与 队列绑定信息的映射关系，这个 map 中包含了所有绑定信息，并且以交换机为单元进行了区分
    // 交换机与队列的关系
    using BindingMap = std::unordered_map<std::string, MsgQueueBindingMap>;
    // 采用以上两个结构 则删除交换机相关绑定信息的 不仅要删除交换机映射 还要删除对应队列中的映射 否则对象得不到释放

    // 持久化信息管理类
    class BindingMapper
    {
    public:
        BindingMapper(const std::string &dbfile) :_sql_helper(dbfile) {}
        
        // 创建数据库表
        void createTable();
        
        // 删除数据库表
        void removeTable();

        // 添加绑定信息
        bool insert(Binding::ptr &binding);

        // 移除绑定信息
        void remove(const std::string &ename, const std::string &qname);
        
        // 移除交换机绑定信息
        void removeExchangeBindings(const std::string &ename);

        // 移除队列绑定信息
        void removeMsgQueueBindings(const std::string &qname);

        // 获取所有数据(恢复历史数据)
        BindingMap recovery();
    private:
        SqliteHelper _sql_helper;
    };
}

#endif