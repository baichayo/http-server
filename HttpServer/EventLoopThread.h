#ifndef EVENTLOOPTHREAD_H
#define EVENTLOOPTHREAD_H

#include "EventLoop.h"

class EventLoopThread : noncopyable
{
public:
    EventLoopThread();
    ~EventLoopThread();
    EventLoop *startLoop();

private:
    void threadFunc();
    EventLoop *loop_;
    bool exiting_;
    Thread thread_;
    MutexLock mutex_;
    Condition cond_;
};

#endif // EVENTLOOPTHREAD_H