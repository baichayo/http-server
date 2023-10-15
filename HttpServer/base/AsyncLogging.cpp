#include "AsyncLogging.h"
#include "LogFile.h"

#include <assert.h>

// 其实可以看作一个 生产者-消费者 模型，
// append() 生产日志，往 vector 中 存日志，
// threadFunc 消费日志，往 vector 中 取日志写到文件里去
// 当 消费者跟不上生产者速度时（积压太多日志），消费者直接把积压的日志丢了
// 为了节省拷贝，析构 BufferVector 带来的开销，用了 std::move()

// 不过我认为可以用（手写）循环队列，避免 vector 可能会重新分配内存 带来的开销

AsyncLogging::AsyncLogging(std::string basename, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1)
{
    assert(basename.size() > 1);
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char *logline, int len)
{// 看这个策略，容易造成的后果：
 //     1. 不同的线程的日志写入顺序不可控
 //     2. 没写的日志直接丢了？（只是用 nextB 代替了 curB ，nextB 中的日志直接被清掉了）
 //     3. 日志没丢，要写的日志会被放进 buffers ，nextB 是备用缓冲区，是空的
    MutexLockGuard lock(mutex_);
    if(currentBuffer_->avail() > len)
        currentBuffer_->append(logline, len);
    else
    {
        //buffers_.push_back(currentBuffer_);
        //currentBuffer_.reset(); // 这里调用的是 shared_ptr 的成员函数，释放 shared_ptr 管理的对象

        // currentBuffer_ 释放了 Buffer 对象
        // 不过 buffers 接着管理了 Buffer 对象
        buffers_.push_back(std::move(currentBuffer_));

        if(nextBuffer_) currentBuffer_ = std::move(nextBuffer_); // nextB 管理的对象交由 curB 接管
        else currentBuffer_.reset(new Buffer); // Rarely happens

        currentBuffer_->append(logline, len);
        cond_.notify();
    }
}

void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();

    LogFile output(basename_);
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while(running_)
    {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());

        {
            MutexLockGuard lock(mutex_);
            if(buffers_.empty()) // 当日志量小的时候，延迟会儿再写
                cond_.waitForSeconds(flushInterval_);

            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);

            if(!nextBuffer_)
                nextBuffer_ = std::move(newBuffer2);
        }

        assert(!buffersToWrite.empty());

        if(buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log messages %zd larger buffers\n",
                        buffersToWrite.size()-2);
            fputs(buf, stderr);
            output.append(buf, static_cast<int>(strlen(buf)));
            // 只留下两块缓冲区
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        for (const auto& buffer : buffersToWrite)
            output.append(buffer->data(), buffer->length());
        
        if(buffersToWrite.size() > 2) // drop non-bzero-ed buffers, avoid trashing
            buffersToWrite.resize(2);
        
        if(!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }

    output.flush();
}