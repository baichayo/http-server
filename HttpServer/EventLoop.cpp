#include "EventLoop.h"
#include "Util.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <assert.h>

typedef std::shared_ptr<Channel> SP_Channel;

__thread EventLoop *t_loopInThisThread = 0;

//用于多线程或多进程间的异步事件通知，
//允许一个线程或进程等待另一个线程或进程的事件发生，而无需使用显式的锁来同步
int createEventfd() // 唤醒 阻塞的 epoll_wait() ，一般做法是往管道写一个字节
{
    int evtfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG << "Failed in eventfd";
        abort();
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false), poller_(new Epoll()), wakeupFd_(createEventfd()),
      quit_(false), eventHandling_(false), callingPendingFunctors_(false),
      threadId_(CurrentThread::tid()),
      pwakeupChannel_(new Channel(this, wakeupFd_))
{
    if(t_loopInThisThread)
    {
         LOG << "Another EventLoop " << " exists in this \
            thread " << static_cast<int>(threadId_);
        assert(t_loopInThisThread == NULL);
    }
    else t_loopInThisThread = this;

    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
    pwakeupChannel_->setReadHandler(std::bind(&EventLoop::handleRead, this));
    pwakeupChannel_->setConnHandler(std::bind(&EventLoop::handleConn, this));
    poller_->epoll_add(pwakeupChannel_, 0);
}

EventLoop::~EventLoop()
{
    // wakeupChannel_->disableAll();
    // wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = NULL;
}

void EventLoop::handleConn()
{
    // poller_->epoll_mod(wakeupFd_, pwakeupChannel_, (EPOLLIN | EPOLLET |
    // EPOLLONESHOT), 0);
    updatePoller(pwakeupChannel_, 0);
}


void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = writen(wakeupFd_, (char *)(&one), sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead()
{
    uint64_t one = 1;
    ssize_t n = readn(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
    // pwakeupChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
    pwakeupChannel_->setEvents(EPOLLIN | EPOLLET);
}

void EventLoop::runInLoop(Functor &&cb)
{
    if (isInLoopThread())
        cb();
    else
        queueInLoop(std::move(cb));
}

void EventLoop::queueInLoop(Functor &&cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_)
        wakeup();
}

void EventLoop::loop()
{
    assert(!looping_);
    assert(isInLoopThread());
    looping_ = true;
    quit_ = false;
    // LOG_TRACE << "EventLoop " << this << " start looping";
    std::vector<SP_Channel> ret;
    while (!quit_)
    {
        // cout << "doing" << endl;
        ret.clear();
        ret = poller_->epoll_wait();
        eventHandling_ = true;
        for (auto &it : ret)
            it->handleEvents();
        eventHandling_ = false;
        doPendingFunctors();
        poller_->handleExpired();
    }
    looping_ = false;
}

void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (size_t i = 0; i < functors.size(); ++i)
        functors[i]();
    callingPendingFunctors_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        wakeup();
    }
}