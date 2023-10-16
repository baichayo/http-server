#ifndef TIMER_H
#define TIMER_H

#include "base/MutexLock.h"

#include <memory>
#include <vector>
#include <queue>
#include <algorithm>

class HttpData;

class TimerNode;
struct TimerCmp
{
    bool operator()(std::shared_ptr<TimerNode> &a,
                    std::shared_ptr<TimerNode> &b) const
    {
        return a->getExpTime() > b->getExpTime();
    }
};

class TimerNode
{
public:
    TimerNode(std::shared_ptr<HttpData> requestData, int timeout);
    TimerNode(const TimerNode &tn);
    ~TimerNode();

    void update(int timeout);
    bool isValid();
    void clearReq();
    void setDeleted() { deleted_ = true; }
    bool isDeleted() const { return deleted_; }
    size_t getExpTime() const { return expiredTime_; }

private:
    bool deleted_;
    size_t expiredTime_;
    std::shared_ptr<HttpData> SPHttpData;
};

class TimerManager
{
public:
    TimerManager();
    ~TimerManager();
    void addTimer(std::shared_ptr<HttpData> SPHttpData, int timeout);
    void handleExpiredEvent();

private:
    typedef std::shared_ptr<TimerNode> SPTimerNode;
    std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp>
        timerNodeQueue;
    // MutexLock lock;
};

#endif // TIMER_H