#ifndef MUTEXLOCK_H
#define MUTEXLOCK_H

#include "noncopyable.h"

#include <pthread.h>
#include <assert.h>

class MutexLock : noncopyable
{
public:
    MutexLock() { pthread_mutex_init(&mutex_, NULL); }
    ~MutexLock()
    {
        pthread_mutex_lock(&mutex_);
        pthread_mutex_destroy(&mutex_);
    }

    void lock() { pthread_mutex_lock(&mutex_); }
    void unlock() { pthread_mutex_unlock(&mutex_); }

    pthread_mutex_t *get() { return &mutex_; }

private:
    pthread_mutex_t mutex_;

private:
    friend class Condition;
};

class MutexLockGuard : noncopyable
{
public:
    MutexLockGuard(MutexLock &m) : mutex_(m) { mutex_.lock(); }
    ~MutexLockGuard() { mutex_.unlock(); }

private:
    MutexLock &mutex_;
};

#endif // MUTEXLOCK_H