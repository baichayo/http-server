<h1>WebServer 总结</h1>

本项目参考自  [[A C++ High Performance Web Server](https://github.com/linyacool/WebServer#a-c-high-performance-web-server)] 和 [陈硕的 muduo 网络库](https://github.com/chenshuo/muduo) 。

本项目为实现了一个简单的Web服务器，支持 get 请求，可处理静态资源。



## 项目构成

项目主要由以下几个部分构成：

- 用 RAII （资源获取即初始化）思想封装的 锁 和 状态变量，以方便地管理多线程。
- 封装了线程和实现了线程库：以方便和安全地使用线程。
- 日志库：实现了异步日志功能，采用了 C++ stream 风格，使用起来简单。
- 定时器： 使用基于小根堆的定时器管理定时事件。
- 状态机：用于解析HTTP请求，支持 get 请求。
- 网络库：自动管理连接请求，避免和 socket 打交道。



## 采用的技术

- 基于 生产者-消费者 思想 ，使用**双缓冲技术**实现 异步日志系统
- 使用**多线程**充分利用多核CPU，并使用**线程池**避免频繁创建和销毁线程造成的开销
- 使用**智能指针**管理对象，减少资源泄露的风险
- 使用 **I/O 复用**技术（Epoll + Edge Trigger ）+ **非阻塞 I/O** ，实现 Reactor 模式
- 利用了 STL 的优先队列实现定时器关闭超时请求
- 使用状态机解析HTTP请求，分为主状态机和从状态机
- 主线程只负责 监听套接字 accept 请求，使用消息队列和 Round Robin 将请求分发给 I/O 线程
- 使用eventfd实现了线程的异步唤醒
- 设置连接上限，避免文件描述符耗尽，导致客户端忙等



## 详细设计

### 日志库

日志库由五部分构成：FileUtil  LogFile  LogStream  AsyncLogging   Logging

#### FileUtil  和 LogFile

FileUtil  封装了文件的打开和写入操作，这是最底层的类。

LogFile  则进一步封装了 FileUtil ，对文件操作加锁，避免竞态条件。并且每写几次就调用 flush ，确保日志的及时性。

#### LogStream  

LogStream 有两个类：FixedBuffer 和 LogStream 。

FixedBuffer 的作用是封装了一个缓冲区，体现了 RAII 思想，有效地避免内存泄漏，也大大地方便了缓冲区的使用，往缓冲区写东西只需调用 append() 即可。

LogStream 则主要是重载 `<<` 运算符，用户调用 `<<` 即可向缓冲区写入东西。

#### AsyncLogging 

AsyncLogging 是异步日志库的核心。AsyncLogging 对外表现为 双缓冲区，但实际上可以看作生产者-消费者模型，缓冲区上限为 25 （超过这个上限大概率是程序遇到bug导致疯狂写日志）。用 `vector` + `shared_ptr` 管理缓冲区。为了避免对象的拷贝，使用了 `std::move` 。

AsyncLogging  会单独占用一个线程，不会阻塞主线程 I/O 。

AsyncLogging 可以看作 日志系统的后端，负责将缓冲区的内容写进文件。

#### Logging

Logging 最终实现了日志功能。用户需要打日志，只需要一条语句即可。

```
LOG << "log something";
```

这条语句会生成临时的 Logger 对象，并返回 成员 LogStream 对象。因此 依靠重载的 `<<` 实现往缓冲区内写东西。

Logging 还会给日志加上时间戳和打日志的代码位置。在生成临时对象时，先加上时间戳，待语句调用完毕，临时对象消失调用析构函数，再往缓冲区加上代码位置，最后调用 `AsyncLogger_->append()` 写日志。代码无不体现了 RAII 思想，科学地管理对象。

为了保证全局只有一个 AsyncLogging 对象，`pthread_once` 。

可以将 logging 看作日志系统的前端，负责将东西写进缓冲区。



### Reactor 模式

这是该项目的核心结构。构成 Reactor 模式的主要有三部分：EventLoop  Channel 和 Epoll

#### Epoll

Epoll 是IO multiplexing的封装，管理连接和事件。采用了 I/O复用 + 非阻塞 I/O ，当对应事件发生时，通知给 Channel。

#### Channel

Channel 可以看作是连接的封装，负责处理发生的监听事件。对应的事件发生时，会调用用户注册的回调函数来完成相应的动作。

#### EventLoop  

该项目采用的是 one loop per thread 思想，EventLoop 做到了这点。如果在一个线程创建多个 EventLoop 对象，程序会直接报错退出。

创建 EventLoop 对象时，会跟着创建 Epoll 对象 和 用来唤醒线程的 Channel 对象 pwakeupChannel_。

EventLoop 只会工作在 IO 线程，这样能避免锁的争用。如果 EventLoop 不在 IO 线程，EventLoop 会退出什么也不做。

当一个连接到来时，会accept 分配文件描述符，并分配Channel对象，将 Channel 对象加入 Epoll 监听队列，事件发生时，Channel 根据所发生事件调用回调函数。



EventLoop  Channel 和 Epoll 构成了简单的 Reactor 模式，值得注意的是，对于用户来说，使用它更像 Proactor 模式。用户并不关心事件什么时候发生，用户只需将要完成的操作告诉程序，待相应事件发生时，程序会自动调用相应操作。



### 定时器

使用基于小根堆的定时器关闭超时请求，直接用了 STL 的 priority_queue 。增加定时事件和删除定时事件的复杂度都是 O(nlogn) 。



### HTTP状态机

虽然这是一个 webserver ，但这个项目目的在于属于网络编程，解析 HTTP 并没自己实现。



### 线程池

线程池采用了最基础的 Round Robin 方式分配任务。



## 项目难点

- 对于多线程中可能产生的竞态条件仍然分析不来。什么时候该加锁，锁的区域大小多少合适仍然分析不清楚。
- 对象的管理：该项目的耦合度比较高，对象互相嵌套，虽然利用智能指针管理了对象来避免资源泄露等问题，但对象的生命周期也更难分析了。
- 还有一些实现看不懂，如负责保存线程信息的 CurrentThread



## 项目待改进的地方

- 定时器是基于优先队列实现的，并且采用的是惰性删除，时间精度不是很高，之后可以换成基于 Timing Wheel 的实现，可以有效减少复杂度。
- 线程池使用了最简单的 Round Robin 方式分配任务，以后或许可以添加算法做到负载均衡。
- 非阻塞IO 应该搭配 应用层 buffer 避免阻塞，该项目尚未实现。



## 收获

- 让我对 RAII 思想有了充分的理解，利用 RAII 风格编写代码能有效管理对象。
- 实践了许多 C++11 的特性，如智能指针，移动拷贝。
- 学会了如何打日志。
- 对网络连接的管理有了更加深刻的认识。在网络中可能会遇到串话，粘包问题。
- 学会了 makefile 和 Cmake