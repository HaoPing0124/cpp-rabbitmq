#ifndef __M_HELPER_H__
#define __M_HELPER_H__

#include <sqlite3.h>
#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <random>
#include <sstream>
#include <atomic>
#include <iomanip>
#include <cerrno>
#include <sys/stat.h>
#include "mq_logger.hpp"

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
                ELOG("创建/打开sqlite数据库失败: %s", sqlite3_errmsg(_handler));
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
        static size_t split(const std::string &str, const std::string &sep, std::vector<std::string> &result)
        {
            size_t idx = 0, pos = 0;
            while (idx < str.size())
            {
                pos = str.find(sep, idx);
                // 没有找到,则从查找位置截取到末尾
                if (pos == std::string::npos)
                {
                    result.push_back(str.substr(idx));
                    return result.size();
                }

                // pos == idx 代表两个分隔符之间没有数据，或者说查找起始位置就是分隔符
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

    class UUIDHelper
    {
    public:
        static std::string uuid()
        {
            std::random_device rd;
            // size_t num = rd(); // 生成一个机器随机数，效率较低
            // 因此解决方案，就是通过一个机器随机数作为生成伪随机数的种子
            std::mt19937_64 gernator(rd()); // 通过梅森旋转算法，生成一个伪随机数

            // 我们要生成的是8个0~255之间的数字，所以要限定数字区间
            std::uniform_int_distribution<int> distrbution(0, 255);
            std::stringstream ss;
            for (int i = 0; i < 8; i++)
            {
                // 将生成的数字转换为16进制数字字符, 填充为2位字符，填充字符为'0'
                ss << std::setw(2) << std::setfill('0') << std::hex << distrbution(gernator);
                if (i == 3 || i == 5 || i == 7)
                {
                    ss << "-";
                }
            }

            // 定义一个原子类型整数，初始化为1
            static std::atomic<size_t> seq(1);
            size_t num = seq.fetch_add(1);
            for (int i = 7; i >= 0; i--)
            {
                ss << std::setw(2) << std::setfill('0') << std::hex << ((num >> (i * 8)) & 0xff);
                if (i == 6)
                    ss << "-";
            }
            return ss.str();
        }
    };

    class FileHelper
    {
    public:
        FileHelper(const std::string &filename) : _filename(filename)
        {
        }

        // 判断文件是否存在
        bool exists()
        {
            struct stat st;
            return (stat(_filename.c_str(), &st) == 0);
        }

        // 文件大小获取
        size_t size()
        {
            struct stat st;
            int ret = stat(_filename.c_str(), &st);
            if (ret < 0)
            {
                return 0;
            }
            return st.st_size;
        }

        // 文件读/写
        // 读写重载，参数：2.偏移量  3.长度
        bool read(std::string &body)
        {
            // 获取文件大小，根据文件大小调整body的空间
            size_t fsize = this->size();
            body.resize(fsize);
            return read(&body[0], 0, fsize);
        }

        bool read(char *body, size_t offset, size_t len)
        {
            // 1. 打开文件
            // 参数：1.文件名 2.打开规则( std::ios::binary 二进制方式打开)
            // ifstream(const char *__s, std::ios_base::openmode __mode)
            std::ifstream ifs(_filename, std::ios::binary | std::ios::in);
            if (!ifs.is_open())
            {
                ELOG("%s 文件打开失败！", _filename.c_str());
                ifs.close();
                return false;
            }

            // 2. 跳转文件读写位置
            // 参数：1.偏移量 2.起始位置
            // seekg(std::streamoff, std::ios_base::seekdir)
            ifs.seekg(offset, std::ios::beg);

            // 3. 读取文件数据
            // 参数：1.读取文件 2.读取长度
            // read(char *__s, std::streamsize __n)
            ifs.read(body, len);
            if (!ifs.good())
            {
                ELOG("%s 文件读取数据失败！", _filename.c_str());
                ifs.close();
                return false;
            }

            // 4. 关闭文件
            ifs.close();
            return true;
        }

        bool write(const std::string &body)
        {
            return write(body.c_str(), 0, body.size());
        }

        bool write(const char *body, size_t offset, size_t len)
        {
            // 1. 打开文件
            std::fstream fs(_filename, std::ios::binary | std::ios::in | std::ios::out);
            if (!fs.is_open())
            {
                ELOG("%s 文件打开失败！", _filename.c_str());
                fs.close();
                return false;
            }

            // 2. 跳转到文件指定位置
            fs.seekp(offset, std::ios::beg);

            // 3. 写入数据
            fs.write(body, len);
            if (!fs.good())
            {
                ELOG("%s 文件写入数据失败！", _filename.c_str());
                fs.close();
                return false;
            }

            // 4. 关闭文件
            fs.close();
            return true;
        }

        // 修改文件名称
        bool rename(const std::string &nname)
        {
            return (::rename(_filename.c_str(), nname.c_str()) == 0);
        }

        // 文件创建/删除
        static bool createFile(const std::string &filename)
        {
            std::fstream ofs(filename, std::ios::binary | std::ios::out);
            if (ofs.is_open() == false)
            {
                ELOG("%s 文件打开失败！", filename.c_str());
                return false;
            }
            ofs.close();
            return true;
        }

        static bool removeFile(const std::string &filename)
        {
            return (::remove(filename.c_str()) == 0);
        }

        // 目录创建/删除
        static bool createDirectory(const std::string &path)
        {
            //  aaa/bbb/ccc    cccc
            // 在多级路径创建中，我们需要从第一个父级目录开始创建
            size_t pos, idx = 0;
            while (idx < path.size())
            {
                pos = path.find("/", idx);
                if (pos == std::string::npos)
                {
                    return (mkdir(path.c_str(), 0775) == 0);
                }
                std::string subpath = path.substr(0, pos);
                int ret = mkdir(subpath.c_str(), 0775);
                if (ret != 0 && errno != EEXIST)
                {
                    ELOG("创建目录 %s 失败: %s", subpath.c_str(), strerror(errno));
                    return false;
                }
                idx = pos + 1;
            }
            return true;
        }

        static bool removeDirectory(const std::string &path)
        {
            // rm -rf path
            // system()
            std::string cmd = "rm -rf " + path;
            return (system(cmd.c_str()) != -1);
        }

        // 获取文件的父级目录
        static std::string parentDirectory(const std::string &filename)
        {
            // /aaa/bb/ccc/ddd/test.txt
            size_t pos = filename.find_last_of("/");
            if (pos == std::string::npos)
            {
                // test.txt
                return "./";
            }
            std::string path = filename.substr(0, pos);
            return path;
        }

        ~FileHelper() {}

    private:
        std::string _filename;
    };
}
#endif