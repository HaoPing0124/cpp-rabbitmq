#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int Add(int num1, int num2)
{ 
    std::cout << "加法执行" << std::endl;
    return num1 + num2; 
}

int main()
{
    // std::async(func, ...)    std::async(policy, func, ...)
    std::cout << "-----------1-----------" << std::endl;

    // std::launch::deferred：在执行get获取异步结果时 才会执行异步任务
    // std::launch::async：内部会创建工作线程 异步地完成任务
    // std::future<int> result = std::async(std::launch::deferred, Add, 11, 22);
    std::future<int> result = std::async(std::launch::async, Add, 11, 22);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "-----------2-----------" << std::endl;
    int sum = result.get();
    
    std::cout << "-----------3-----------" << std::endl;
    std::cout << sum << std::endl;
    
    return 0;
}