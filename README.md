# 仿RabbitMQ轻量级消息队列系统

#### 介绍
基于C++实现仿RabbitMQ的轻量级消息队列
采用生产者-消费者模型完成跨主机异步通信
项目基于muduo网络库与epoll事件驱动模型，实现高并发TCP长连接通信；
使用Protobuf完成消息二进制序列化，自定义应用层协议实现消息收发；
通过SQLite3持久化交换机、队列等元数据，结合Gtest完成模块单元测试
项目运行于Linux环境，具备消息解耦、削峰填谷、异步处理等典型MQ特性。

#### 软件架构
软件架构说明


#### 安装教程

1.  xxxx
2.  xxxx
3.  xxxx

#### 使用说明

1.  xxxx
2.  xxxx
3.  xxxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
