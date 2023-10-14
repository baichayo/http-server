#ifndef CONDITION_H
#define CONDITION_H

#include "MutexLock.h"

#include <errno.h>

class Condition : noncopyable
{
public:
    Condition(MutexLock &m) : mutex_(m) { pthread_cond_init(&cond_, NULL); }
    ~Condition() { pthread_cond_destroy(&cond_); }

    void wait() { pthread_cond_wait(&cond_, mutex_.get()); }
    void notify() { pthread_cond_signal(&cond_); }
    void notifyAll() { pthread_cond_broadcast(&cond_); }

    bool waitForSeconds(int seconds)
    {
        struct timespec abstime;
        clock_gettime(CLOCK_REALTIME, &abstime);
        abstime.tv_sec += static_cast<time_t>(seconds);
        return ETIMEDOUT == pthread_cond_timedwait(&cond_, mutex_.get(), &abstime);
    }

private:
    pthread_cond_t cond_;
    MutexLock &mutex_;
};

#endif // CONDITION_H