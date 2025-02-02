#ifndef FILEUTIL_H
#define FILEUTIL_H

#include "noncopyable.h"

#include <string>

class AppendFile : noncopyable
{
public:
    AppendFile(std::string filename);
    ~AppendFile();

    void append(const char *logline, const size_t len);
    void flush();

private:
    size_t write(const char *logline, size_t len);
    FILE *fp_;
    char buffer_[64 * 1024];
};

#endif // FILEUTIL_H