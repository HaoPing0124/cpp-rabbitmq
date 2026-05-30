/*
    packged_task的使用
        packged_task 是一个模版类，实例化的对象可以对一个函数进行二次封装
    packged_task 可以通过 get_future() 获取一个future对象，来获取封装这个函数的异步执行结果
*/
#include <iostream>
#include <thread>
#include <future>
#include <memory>

int Add(int num1, int num2)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return num1 + num2;
}

int main()
{
    // int Add(int, int)
    // std::packaged_task<int(int, int)> task(Add);
    // std::future<int> fu = task.get_future();

    // task可以当做一个可调用对象来调用执行任务
    // 但是又不能完全当做一个函数使用
    // task(11, 22);
    // std::async(std::launch::async, task, 11, 22);
    // std::thread thr(task, 11, 22);

    // 可以将task定义为一个指针，传递到线程中，然后进行解引用执行
    // 如果单纯指针指向一个对象，存在生命周期问题，很有可能出现风险
    // 思想就是在堆上 new 一个对象，用智能指针管理它的生命周期
    auto ptask = std::make_shared<std::packaged_task<int(int, int)>>(Add);
    std::future<int> fu = ptask->get_future();
    std::thread thr([ptask](){
        (*ptask)(11, 22);
    });

    int sum = fu.get();
    std::cout << "sum:" << sum << std::endl;
    thr.join();
    return 0;
}