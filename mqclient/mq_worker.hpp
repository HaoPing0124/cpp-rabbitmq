#ifndef __M_CLIENT_WORKER_H__
#define __M_CLIENT_WORKER_H__
#include "muduo/net/EventLoopThread.h"
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_threadpool.hpp"

namespace haoping
{
    // 异步工作模块
    // 将 muduo 的事件循环线程和项目中的业务线程池组合在一起
    // loopthread 负责运行网络事件循环
    // pool 负责执行消息处理等异步业务任务
    class AsyncWorker
    {
    public:
        using ptr = std::shared_ptr<AsyncWorker>;

        // muduo 事件循环线程
        // 负责在独立线程中运行 EventLoop
        // 可以用于处理客户端连接、网络读写和消息分发等网络事件
        muduo::net::EventLoopThread loopthread;

        // 异步业务任务线程池
        // 负责执行不适合直接放在网络线程中处理的业务任务
        // 避免耗时业务阻塞 muduo 的事件循环线程
        ThreadPool pool;
    };
}

#endif