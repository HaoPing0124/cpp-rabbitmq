#include "sqlite.hpp"
#include <iostream>
#include <cassert>
#include <vector>

// 数据库每行处理一次回调函数
// 查询回调函数 参数：1.传入的 2.列的数量 3.查询结果 4.字段名称
int select_stu_callback(void *arg, int col_count, char **result, char **fields_name)
{
    std::vector<std::string> *arry = (std::vector<std::string> *)arg;
    arry->push_back(result[0]);   
    return 0;  
}

int main()
{
    SqliteHelper handler("./test.db");
    assert(handler.open());

    const char *ct = "create table if not exists student(sno char(10) primary key, name varchar(30), age int)";
    assert(handler.exec(ct, nullptr, nullptr));

    const char *insert_sql = "insert into student values('001', '张三', 18), ('002', '李四', 20), ('003', '王五', 21)";
    assert(handler.exec(insert_sql, nullptr, nullptr));

    const char *update_sql = "update student set name = '张小明' where sno = '001'";
    assert(handler.exec(update_sql, nullptr, nullptr));

    const char *delete_sql = "delete from student where sno = '003'";
    assert(handler.exec(delete_sql, nullptr, nullptr));

    // const char *select_sql = "select sno, name from student";
    const char *select_sql = "select name from student";
    std::vector<std::string> arry;
    assert(handler.exec(select_sql, select_stu_callback, &arry));
    for(auto &name : arry){
        std::cout << name << std::endl;
    }

    handler.close();
    return 0;
}