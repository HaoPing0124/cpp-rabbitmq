# 仿RabbitMQ轻量级消息队列系统

#### Description
基于C++实现仿RabbitMQ的轻量级消息队列
采用生产者-消费者模型完成跨主机异步通信
项目基于muduo网络库与epoll事件驱动模型，实现高并发TCP长连接通信；
使用Protobuf完成消息二进制序列化，自定义应用层协议实现消息收发；
通过SQLite3持久化交换机、队列等元数据，结合Gtest完成模块单元测试
项目运行于Linux环境，具备消息解耦、削峰填谷、异步处理等典型MQ特性。

#### Software Architecture
Software architecture description

#### Installation

1.  xxxx
2.  xxxx
3.  xxxx

#### Instructions

1.  xxxx
2.  xxxx
3.  xxxx

#### Contribution

1.  Fork the repository
2.  Create Feat_xxx branch
3.  Commit your code
4.  Create Pull Request


#### Gitee Feature

1.  You can use Readme\_XXX.md to support different languages, such as Readme\_en.md, Readme\_zh.md
2.  Gitee blog [blog.gitee.com](https://blog.gitee.com)
3.  Explore open source project [https://gitee.com/explore](https://gitee.com/explore)
4.  The most valuable open source project [GVP](https://gitee.com/gvp)
5.  The manual of Gitee [https://gitee.com/help](https://gitee.com/help)
6.  The most popular members  [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
