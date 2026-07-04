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
        static bool route(ExchangeType type, const std::string &routing_key, const std::string &binding_key)
        {
            // 直接交换
            if (type == ExchangeType::DIRECT)
            {
                return routing_key == binding_key;
            }
            // 广播交换
            else if (type == ExchangeType::FANOUT)
            {
                return true;
            }

            // 主题交换：要进行模式匹配    news.#   &   news.music.pop
            // 动态规划
            // 1. 将binding_key与routing_key进行字符串分割，得到各个的单词数组
            std::vector<std::string> bkeys, rkeys;
            int n_bkey = StrHelper::split(binding_key, ".", bkeys);
            int n_rkey = StrHelper::split(routing_key, ".", rkeys);

            // 2. 定义标记数组，并初始化[0][0]位置为true，其他位置为false
            std::vector<std::vector<bool>> dp(n_bkey + 1, std::vector<bool>(n_rkey + 1, false));
            dp[0][0] = true;

            // 3. 如果binding_key以#起始，则将#对应行的第0列置为1.
            for (int i = 1; i <= bkeys.size(); i++)
            {
                if (bkeys[i - 1] == "#")
                {
                    dp[i][0] = true;
                    continue;
                }
                break;
            }

            // 4. 使用routing_key中的每个单词与binding_key中的每个单词进行匹配并标记数组
            for (int i = 1; i <= n_bkey; i++)
            {
                for (int j = 1; j <= n_rkey; j++)
                {
                    // 如果当前bkey是个*，或者两个单词相同，表示单词匹配成功，则从左上方继承结果
                    if (bkeys[i - 1] == rkeys[j - 1] || bkeys[i - 1] == "*")
                    {
                        dp[i][j] = dp[i - 1][j - 1];
                    }
                    else if (bkeys[i - 1] == "#")
                    {
                        // 如果当前bkey是个#，则需要从左上，左边，上边继承结果
                        dp[i][j] = dp[i - 1][j - 1] | dp[i][j - 1] | dp[i - 1][j];
                    }
                }
            }
            return dp[n_bkey][n_rkey];
        }

    private:
    };
}

#endif