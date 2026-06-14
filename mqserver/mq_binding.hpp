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
        std::string exchange_name; // 交换机名称
        std::string msgqueue_name; // 队列名称
        std::string binding_key;   // 绑定信息

        Binding() {}
        Binding(const std::string &ename, const std::string &qname, const std::string &key)
            : exchange_name(ename), msgqueue_name(qname), binding_key(key)
        {
        }
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
        BindingMapper(const std::string &dbfile) : _sql_helper(dbfile)
        {
            std::string path = FileHelper::parentDirectory(dbfile);
            FileHelper::createDirectory(path);
            _sql_helper.open();
            createTable();
        }

        // 创建数据库表
        void createTable()
        {
            // create table if not exists binding_table(exchange_name varchar(32), msgqueue_name, binding_key)
            std::stringstream sql;
            sql << "create table if not exists binding_table(";
            sql << "exchange_name varchar(32), ";
            sql << "msgqueue_name varchar(32), ";
            sql << "binding_key varchar(128));";
            assert(_sql_helper.exec(sql.str(), nullptr, nullptr));
        }

        // 删除数据库表
        void removeTable()
        {
            std::string sql = "drop table if exists binding_table;";
            _sql_helper.exec(sql, nullptr, nullptr);
        }

        // 添加绑定信息
        bool insert(Binding::ptr &binding)
        {
            // insert into binding_table values('exchange1', 'msgqueue1', 'news.music.#');
            std::stringstream sql;
            sql << "insert into binding_table values(";
            sql << "'" << binding->exchange_name << "', ";
            sql << "'" << binding->msgqueue_name << "', ";
            sql << "'" << binding->binding_key << "');";
            return _sql_helper.exec(sql.str(), nullptr, nullptr);
        }

        // 移除绑定信息
        void remove(const std::string &ename, const std::string &qname)
        {
            // delete form binding_table where exchange_name='' and msgqueue_name='';
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "exchange_name='" << ename << "' and ";
            sql << "msgqueue_name='" << qname << "';";
            _sql_helper.exec(sql.str(), nullptr, nullptr);
        }

        // 移除交换机绑定信息
        void removeExchangeBindings(const std::string &ename)
        {
            // delete from binding_table where exchange_name='';
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "exchange_name='" << ename << "';";
            _sql_helper.exec(sql.str(), nullptr, nullptr);
        }

        // 移除队列绑定信息
        void removeMsgQueueBindings(const std::string &qname)
        {
            std::stringstream sql;
            sql << "delete from binding_table where ";
            sql << "msgqueue_name='" << qname << "';";
            _sql_helper.exec(sql.str(), nullptr, nullptr);
        }

        // 获取所有数据(恢复历史数据)
        BindingMap recovery()
        {
            BindingMap result;
            std::string sql = "select exchange_name, msgqueue_name, binding_key from binding_table;";
            _sql_helper.exec(sql, selectCallback, &result);
            return result;
        }

    private:
        static int selectCallback(void *arg, int numcol, char **row, char **fields)
        {
            BindingMap *result = (BindingMap *)arg;
            Binding::ptr bp = std::make_shared<Binding>(row[0], row[1], row[2]);
            // 为了防止 交换机相关的绑定信息已经存在，因此不能直接创建队列映射 进行添加 这样会覆盖历史数据
            // 因此得先获取交换机对应的映射对象 往里面添加数据
            // 但是 若这时候没有交换机对应的映射信息 因此这里的获取要使用引用（会保证不存在则自动创建）
            MsgQueueBindingMap &qmap = (*result)[bp->exchange_name];
            qmap.insert(std::make_pair(bp->msgqueue_name, bp));
            return 0;
        }

    private:
        SqliteHelper _sql_helper;
    };

    class BindingManager
    {
    public:
        BindingManager(const std::string &dbfile)
            : _mapper(dbfile)
        {
            _bindings = _mapper.recovery();
        }

        // 增加绑定信息
        bool bind(const std::string &ename, const std::string &qname, const std::string &key, bool durable)
        {
            // 加锁 构造一个队列的绑定信息对象 添加映射关系
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _bindings.find(ename);
            if (it != _bindings.end() && it->second.find(qname) != it->second.end())
            {
                return true;
            }
            // 绑定信息是否需要持久化 取决于 交换机数据是持久化的 以及队列数据也是持久化的
            Binding::ptr bp = std::make_shared<Binding>(ename, qname, key);
            if (durable)
            {
                bool ret = _mapper.insert(bp);
                if (ret == false)
                    return false;
            }
            auto &qbmap = _bindings[ename];
            qbmap.insert(std::make_pair(qname, bp));
            return true;
        }

        // 解除绑定信息
        bool unBind(const std::string &ename, const std::string &qname)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto eit = _bindings.find(ename);
            if (eit == _bindings.end())
            {
                // 没有交换机相关的绑定信息
                return;
            }
            auto qit = eit->second.find(qname);
            if (qit == eit->second.end())
            {
                // 交换机没有队列相关的绑定信息
                return;
            }
            _mapper.remove(ename, qname);
            _bindings[ename].erase(qname);
        }

        // 移除交换机绑定信息
        void removeExchangeBindings(const std::string &ename)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _mapper.removeExchangeBindings(ename);
            _bindings.erase(ename);
        }

        // 移除队列绑定信息
        void removeQueueBindings(const std::string &qname)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _mapper.removeMsgQueueBindings(qname);
            for (auto start = _bindings.begin(); start != _bindings.end(); ++start)
            {
                // 遍历每个交换机的绑定信息，从中移除指定队列的相关信息
                start->second.erase(qname);
            }
        }

        // 获取队列绑定信息
        MsgQueueBindingMap getExchangeBindings(const std::string &ename)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 在整体 绑定关系信息中 查找交换机
            auto eit = _bindings.find(ename);
            if (eit == _bindings.end())
            {
                // 若没有该交换机则返回空值
                return MsgQueueBindingMap();
            }
            // 否则返回交换机对应的绑定信息(队列)
            return eit->second;
        }

        // 获取整体绑定信息(交换机对应绑定关系)
        Binding::ptr getBinding(const std::string &ename, const std::string &qname)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            // 在整体 绑定关系信息中 查找交换机
            auto eit = _bindings.find(ename);
            if (eit == _bindings.end())
            {
                return Binding::ptr();
            }

            // 在交换机对应队列 关系信息中 查找队列
            auto qit = eit->second.find(qname);
            if (qit == eit->second.end())
            {
                return Binding::ptr();
            }
            return qit->second;
        }

        bool exists(const std::string &ename, const std::string &qname);

        size_t size();

        void clear();

    private:
        std::mutex _mutex;
        BindingMapper _mapper; // 持久化管理
        BindingMap _bindings;  // 绑定关系
    };
}

#endif