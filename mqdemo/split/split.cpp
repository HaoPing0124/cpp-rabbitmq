/*
    字符串分割工具类封装
    分割的思想：
        1. 从0位置开始查找指定字符的位置，找到之后进行分割
        2. 从上次查找的位置继续向后查找指定字符
    例：news....music.#.pop
*/
#include <iostream>
#include <string>
#include <vector>

size_t split(const std::string &str, const std::string &sep, std::vector<std::string> &result)
{
    size_t idx = 0, pos = 0;
    while (idx < str.size())
    {
        pos = str.find(sep, idx);
        // 没有找到
        if (pos == std::string::npos)
        {
            // 从查找位置截取到末尾
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

int main()
{
    std::string str = "news....music.#.pop";
    std::vector<std::string> arry;
    int n = split(str, ".", arry);
    for (auto &s : arry)
    {
        std::cout << s << std::endl;
    }

    return 0;
}