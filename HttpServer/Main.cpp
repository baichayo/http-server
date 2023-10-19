#include "Server.h"

#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{
    int threadNum = 4;
    int port = 80;
    std::string logPath = "./WebServer.log";

    // parse args
    int opt;
    const char *str = "t:l:p:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 't':
        {
            threadNum = atoi(optarg);
            break;
        }
        // case 'l':
        // {
        //     logPath = optarg;
        //     if (logPath.size() < 2 || optarg[0] != '/')
        //     {
        //         printf("logPath should start with \"/\"\n");
        //         abort();
        //     }
        //     break;
        // }
        case 'p':
        {
            port = atoi(optarg);
            break;
        }
        default:
            break;
        }
    }
    Logger::setLogFileName(logPath);
// STL库在多线程上应用
#ifndef _PTHREADS
    LOG << "_PTHREADS is not defined !";
#endif
    EventLoop mainLoop;
    Server myHTTPServer(&mainLoop, threadNum, port);
    myHTTPServer.start();
    mainLoop.loop();
    return 0;
}

/*
EventLoop mainLoop;
    poller_(new Poller) : 创建 epfd
    pwakeupChannel_(new Channel(this, wakeupFd_)) : 给唤醒线程用的 fd 封装成 Channel

    往 poller_ 注册 pwakeupChannel_

Server myHTTPServer(&mainLoop, threadNum, port);
    eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)) ：
        生成线程池了
    acceptChannel_(new Channel(loop_)) ：用于封装 listenfd_
    listenFd_(socket_bind_listen(port_)) ：初始化套接字并开始监听

    acceptChannel_->setFd(listenFd_) ：nothing

 myHTTPServer.start();
    eventLoopThreadPool_->start() ：生成新线程，并 start()
    给 acceptChannel_ 设置 回调函数，并将 acceptChannel_ 加进 epfd

mainLoop.loop();
    ret = poller_->epoll_wait() ：监听事件发生并返回 activeChannels

来了新连接怎么办？
    会触发 Epoll::epoll_wait() 监听到事件后，最终 acceptChannel_ 调用 回调函数：Server::handNewConn
    Server::handNewConn ：
        1. 接受连接，并分配一个线程给它，采用轮询方式
        2. std::shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd)) ：
            channel_(new Channel(loop, connfd)) ：给新连接封装成 channel
            给 channel 设置回调函数，为 HttpData 自己的
        3. 将 channel 加进 epfd

http 连接的 回调函数
    HttpData::handleRead ：从 客户端读数据，
        读错了就 调用 handleError，返回 哎呀呀呀
        读不出来数据，可能是中途连接断开了，关闭连接
*/