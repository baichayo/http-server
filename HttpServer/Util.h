#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <string>

ssize_t readn(int fd, void *buff, size_t n);
ssize_t readn(int fd, std::string &inBuffer, bool &zero);
ssize_t readn(int fd, std::string &inBuffer);

ssize_t writen(int fd, void *buff, size_t n);
ssize_t writen(int fd, std::string &sbuff);

void handle_for_sigpipe();

int setSocketNonBlocking(int fd);
void setSocketNodelay(int fd);
void setSocketNoLinger(int fd); // 延迟关闭套接字

void shutDownWR(int fd);
int socket_bind_listen(int port);

#endif // UTIL_H