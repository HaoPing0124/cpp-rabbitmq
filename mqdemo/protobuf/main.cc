#include <iostream>
#include "contacts.pb.h"

int main()
{
    // 序列化
    contacts::contact conn;
    conn.set_sno(10001);
    conn.set_name("小明");
    conn.set_score(99);
    
    // 持久化的数据就存储在str中，此时可以对str进行持久化或网络传输
    std::string str = conn.SerializeAsString();

    // 反序列化
    contacts::contact stu;
    bool ret = stu.ParseFromString(str);
    if(ret == false)
    {
        std::cout << "反序列化失败!\n";
        return -1;
    }

    std::cout << stu.sno() << std::endl;
    std::cout << stu.name() << std::endl;
    std::cout << stu.score() << std::endl;
    return 0;
}