#ifndef __M_ROUTE_H__
#define __M_ROUTE_H__

#include <iostream>
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"

namespace haoping
{
    class Router
    {
    public:
        // 判断 routing_key 是否合法
        static bool isLegalRoutingKey(const std::string &routing_key)
        {
            // routing_key:只需要判断是否包含非法字符即可 合法字符("0~9", "A~Z", "a~z", ".", "_")
            for (auto &ch : routing_key)
            {
                if ((ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') ||
                    (ch == '_' || ch == '.'))
                {
                    continue;
                }
                return false;
            }
            return true;
        }

        // 判断 binding_key 是否合法
        static bool isLegalBindingKey(const std::string &binding_key)
        {
            // 1. 判断是否包含有非法字符， 合法字符：a~z, A~Z, 0~9, ., _, *, #
            for (auto &ch : binding_key)
            {
                if ((ch >= 'a' && ch <= 'z') ||
                    (ch >= 'A' && ch <= 'Z') ||
                    (ch >= '0' && ch <= '9') ||
                    (ch == '_' || ch == '.') ||
                    (ch == '*' || ch == '#'))
                {
                    continue;
                }
                return false;
            }

            // 2. *和#必须独立存在:  news.music#.*.#
            std::vector<std::string> sub_words;
            StrHelper::split(binding_key, ".", sub_words);
            for (std::string &word : sub_words)
            {
                if (word.size() > 1 &&
                    (word.find("*") != std::string::npos ||
                     word.find("#") != std::string::npos))
                {
                    return false;
                }
            }

            // 3. *和#不能连续出现
            for (int i = 1; i < sub_words.size(); i++)
            {
                if (sub_words[i] == "#" && sub_words[i - 1] == "*")
                {
                    return false;
                }
                if (sub_words[i] == "#" && sub_words[i - 1] == "#")
                {
                    return false;
                }
                if (sub_words[i] == "*" && sub_words[i - 1] == "#")
                {
                    return false;
                }
            }
            return true;
        }

        // 路由
        static bool route(ExchangeType type, const std::string &routing_key, const std::string &binding_key);

    private:
    };
}

#endif