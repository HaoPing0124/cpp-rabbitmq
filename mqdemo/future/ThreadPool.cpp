#include <iostream>
#include <vector>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <condition_variable>

using Functor = std::function<void(void)>;
class ThreadPool
{
public:
    ThreadPool(int thr_count = 1) : _stop(false)
    {
        for (int i = 0; i < thr_count; ++i)
        {
            _threads.emplace_back(&ThreadPool::entry, this);
        }
    }
    ~ThreadPool()
    {
        stop();
    }

    // push参数：1.函数(用户要执行的函数) 2.不定参(需要处理的数据也就是要传入到函数的参数)
    // push内部 会将传入的函数封装成一个异步任务(packaged_task)
    // 使用lambda生成一个可调用对象(内部执行异步任务), 抛入到任务池中，再由工作线程取出进行执行
    template <class F, class... Args>
    auto push(F &&func, Args &&...args) -> std::future<decltype(func(args...))>
    {
        // 1.将传入的函数封装成一个packaged_task任务
        using return_type = decltype(func(args...));
        auto tmp_func = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
        auto task = std::make_shared<std::packaged_task<return_type()>>(tmp_func);
        std::future<return_type> fu = task->get_future();

        // 2.构造一个lambda匿名函数(捕获任务对象)，函数内执行任务对象
        {
            std::unique_lock<std::mutex> lock(_mutex);

            // 3.将构造出来的匿名函数 抛入到任务池中
            _taskpool.push_back([task]()
                                { (*task)(); });
            _cv.notify_one();
        }
        return fu;
    }

    void stop()
    {
        _stop = true;
        _cv.notify_all();
        for (auto &thread : _threads)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }
    }

private:
    // 线程入口函数 —— 内部不断从任务池中取出任务进行执行
    void entry()
    {
        while (!_stop)
        {
            std::vector<Functor> tmp_taskpool;
            {
                // 加锁
                std::unique_lock<std::mutex> lock(_mutex);
                // 等待任务池不为空 或者_stop被置位返回
                _cv.wait(lock, [this]()
                         { return _stop || !_taskpool.empty(); });

                // 取出任务进行执行
                tmp_taskpool.swap(_taskpool);
            }
            for (auto &task : tmp_taskpool)
            {
                task();
            }
        }
    }

private:
    std::atomic<bool> _stop;           // 结束标志（原子操作）
    std::vector<Functor> _taskpool;    // 任务池（保存的是packaged_task的匿名函数）
    std::mutex _mutex;                 // 锁
    std::condition_variable _cv;       // 条件变量
    std::vector<std::thread> _threads; // 工作线程
};

int Add(int num1, int num2)
{
    return num1 + num2;
}

int main()
{
    ThreadPool threadpool;
    for (int i = 0; i < 10; ++i)
    {
        std::future<int> fu = threadpool.push(Add, 10, i);
        std::cout << fu.get() << std::endl;
    }
    threadpool.stop();
    return 0;
}