#ifndef THREAD_H
#define THREAD_H

#include "CountDownLatch.h"

#include <functional>
#include <string.h>

class Thread : noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc&, const std::string &name = std::string());
    ~Thread();
    void start();
    int join();

    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string& name() const { return name_; }

private:
    void setDefaultName();
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
};

#endif // THREAD_H