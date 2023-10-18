#ifndef EPOLL_H
#define EPOLL_H

#include "Timer.h"

#include <sys/epoll.h>

class Channel;
class HttpData;

class Epoll
{
public:
    typedef std::shared_ptr<Channel> SP_Channel;

    Epoll();
    ~Epoll();
    void epoll_add(SP_Channel request, int timeout);
    void epoll_mod(SP_Channel request, int timeout);
    void epoll_del(SP_Channel request);
    std::vector<SP_Channel> epoll_wait();
    std::vector<std::shared_ptr<Channel>> getEventsRequest(int events_num);
    void add_timer(std::shared_ptr<Channel> request_data, int timeout);
    int getEpollFd() { return epollFd_; }
    void handleExpired();

private:
    static const int MAXFDS = 100000;
    int epollFd_;
    std::vector<epoll_event> events_;
    std::shared_ptr<Channel> fd2chan_[MAXFDS];
    std::shared_ptr<HttpData> fd2http_[MAXFDS];
    TimerManager timerManager_;
};

#endif // EPOLL_H