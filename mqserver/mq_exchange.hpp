#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace haoping
{
    // 定义交换机类
    struct Exchange
    {
        // 交换机对象
        using ptr = std::shared_ptr<Exchange>;
        // 1.交换机名称
        std::string name;
        // 2.交换机类型
        ExchangeType type; // 直接、广播、主题
        // 3.交换机持久化标志
        bool durable;
        // 4.是否自动删除标志
        bool auto_delete;
        // 5.其他参数
        std::unordered_map<std::string, std::string> args;

        Exchange(const std::string &ename, ExchangeType etype,
                 bool edurable, bool eauto_delete, std::unordered_map<std::string, std::string> &eargs)
            : name(ename), type(etype), durable(edurable),
              auto_delete(eauto_delete), args(eargs)
        {
        }

        // args 会存储一个键值对，在存储数据库时，会组织一个格式字符串进行存储（key=val & key=val）
        // 内部解析 str_args 字符串，将内容存储到成员中
        void setArgs(const std::string &str_args);
        
        // 将 args 中的内容进行序列化后，要返回一个字符串
        std::string getArgs();
    };

    // 定义交换机数据持久化管理类 -- 数据存储在 sqlite 数据库中
    class ExchangeMapper
    {
    public:
        ExchangeMapper(const std::string &dbfile);
        
        // 创建表
        void createTable();
        
        // 删除表
        void removeTable();
        
        // 添加交换机
        void insert(Exchange::ptr &exchange);

        // 移除交换机
        void remove(const std::string &name);
        
        //// 查询单个表(根据名称)
        // Exchange::ptr getOne(const std::string &name);

        // 查询所有表
        std::unordered_map<std::string, Exchange::ptr> getAll(); 
    private:
        SqliteHelper _sql_helper;   // 数据库操作句柄
    };

    // 定义交换机数据内存管理类
    class ExchangeManager
    {
    public:
        ExchangeManager(const std::string &dbfile);
        
        // 声明交换机
        void declareExchange(const std::string &name,
            ExchangeType type, bool durable, bool auto_delete,
            std::unordered_map<std::string, std::string> &args);

        // 删除交换机
        void deleteExchange(const std::string &name);

        //// 交换机查询
        // Exchange::ptr selectExchange(const std::string &name);

        // 判断交换机是否存在
        bool exists(const std::string &name);

        // 清理所有交换机数据
        void clear();

    private:
        std::mutex _mutex;
        ExchangeMapper _mapper;     // 持久化管理
        std::unordered_map<std::string, Exchange::ptr> _exchanges;   // 交换机数据对象管理
    };
}