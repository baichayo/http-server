#ifndef LOGFILE_H
#define LOGFILE_H

#include "FileUtil.h"
#include "Condition.h"

#include <memory>

class LogFile : noncopyable
{
public:
    LogFile(const std::string &basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    //void rollFile();

private:
    void append_unlocked(const char *logline, int len);

    const std::string basename_;
    const int flushEveryN_;

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;
};

#endif // LOGFILE_H