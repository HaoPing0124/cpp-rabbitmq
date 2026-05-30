/*
    封装实现 SqliteHandler类 提供简单的sqlite数据库操作接口 完成数据的基础增删改查操作
        1.创建/打开数据库文件
        2.针对打开的数据库执行操作
            1.表的操作
            2.数据的操作
        3.关闭数据库
*/

#ifndef __M_HELPER_H__
#define __M_HELPER_H__

#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include "logger.hpp"

namespace haoping
{
    class SqliteHelper
    {
        typedef int (*SqliteCallback)(void *, int, char **, char **);

    public:
        SqliteHelper(const std::string &dbfile) : _dbfile(dbfile), _handler(nullptr)
        {
        }

        // 打开数据库
        bool open(int safe_leve = SQLITE_OPEN_FULLMUTEX)
        {
            // 参数：1.文件名称 2.操作句柄 3.打开的标志 4.虚拟文件系统相关信息
            // int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs)
            // SQLITE_OPEN_CREATE(数据库文件不存在则创建) SQLITE_OPEN_READWRITE(以读写方式打开数据库)
            int ret = sqlite3_open_v2(_dbfile.c_str(), &_handler, safe_leve | SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);
            if (ret != SQLITE_OK)
            {
                ELOG("创建/打开sqlite数据库失败: %s", sqlite3_errmsg(_handler))
                return false;
            }
            return true;
        }

        // 执行语句
        // 参数：1.执行操作 2.回调函数 3.保存结果
        bool exec(const std::string &sql, SqliteCallback cb, void *arg)
        {
            // 参数：1.句柄 2.sql语句 3.回调函数 4.传递的参数 5.获取出错时的错误信息
            // int sqlite3_exec(sqlite3 *, const char *sql, int (*callback)(void *, int, char **, char **), void * arg, char **errmsg)
            int ret = sqlite3_exec(_handler, sql.c_str(), cb, arg, nullptr);
            if (ret != SQLITE_OK)
            {
                ELOG("%s\n执行语句失败:%s", sql.c_str(), sqlite3_errmsg(_handler));
                return false;
            }
            return true;
        }

        // 关闭数据库
        void close()
        {
            // int sqlite3_close_v2(sqlite3 *)
            if (_handler)
                sqlite3_close_v2(_handler);
        }

        ~SqliteHelper() {}

    private:
        std::string _dbfile;
        sqlite3 *_handler;
    };

    class StrHelper
    {
    public:
        size_t split(const std::string &str, const std::string &sep, std::vector<std::string> &result)
        {
            size_t idx = 0, pos = 0;
            while (idx < str.size())
            {
                pos = str.find(sep, idx);
                if (pos == std::string::npos)
                {
                    result.push_back(str.substr(idx));
                    return result.size();
                }

                if (pos == idx)
                {
                    idx = pos + sep.size();
                    continue;
                }

                std::string tmp = str.substr(idx, pos - idx);
                result.push_back(tmp);
                idx = pos + sep.size();
            }
            return result.size();
        }
    };
}
#endif