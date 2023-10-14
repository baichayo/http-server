#ifndef CURRENTTHREAD_H
#define CURRENTTHREAD_H

namespace CurrentThread
{
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;

void cacheTid();

//GCC 编译器提供的内建函数（built-in function），
//用于提示编译器关于条件表达式的期望结果，以便优化分支预测
inline int tid() {
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

inline const char* tidString()  // for logging
{
  return t_tidString;
}

inline int tidStringLength()  // for logging
{
  return t_tidStringLength;
}

inline const char* name() { return t_threadName; }

}


#endif // CURRENTTHREAD_H