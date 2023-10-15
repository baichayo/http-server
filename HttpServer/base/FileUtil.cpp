#include "FileUtil.h"


AppendFile::AppendFile(std::string filename)
    : fp_(fopen(filename.c_str(), "ae")) // 'e' for O_CLOEXEC
{
    setbuffer(fp_, buffer_, sizeof buffer_);
}

AppendFile::~AppendFile() { fclose(fp_); }

void AppendFile::append(const char *logline, const size_t len)
{
    size_t remain = len, writen_len = 0;
    while(remain > 0)
    {
        size_t n = this->write(logline + writen_len, remain);
        if(n == 0)
        {
            if(ferror(fp_))
                fprintf(stderr, "AppendFile::append() failed !\n");
            break;
        }
        remain -= n, writen_len += n;
    }
}

void AppendFile::flush() { fflush(fp_); }

size_t AppendFile::write(const char *logline, size_t len)
{
    return fwrite_unlocked(logline, 1, len, fp_);
}