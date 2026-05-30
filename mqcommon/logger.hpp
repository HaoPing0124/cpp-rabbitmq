#ifndef __M_LOG_H__
#define __M_LOG_H__

#include <iostream>
#include <ctime>
#include <unistd.h>

namespace haoping
{
#define DEBUG_LEVEL 0
#define INFO_LEVEL 1
#define ERROR_LEVEL 2

#define DEFAULT_LEVEL DEBUG_LEVEL

#define LOG(lev_str, level, format, ...)                                                                                       \
    {                                                                                                                          \
        if (level >= DEFAULT_LEVEL)                                                                                            \
        {                                                                                                                      \
            time_t t = time(nullptr);                                                                                          \
            struct tm *ptm = localtime(&t);                                                                                    \
            char time_str[32];                                                                                                 \
            strftime(time_str, sizeof(time_str) - 1, "%Y-%m-%d %H:%M:%S", ptm);                                                \
            pid_t pid = getpid();                                                                                              \
            printf("[%s] [%s] [%ld] [%s:%d] - " format "\n", time_str, lev_str, (long)pid, __FILE__, __LINE__, ##__VA_ARGS__); \
        }                                                                                                                      \
    }

#define DLOG(format, ...) LOG("DEBUG", DEBUG_LEVEL, format, ##__VA_ARGS__)
#define ILOG(format, ...) LOG("INFO", INFO_LEVEL, format, ##__VA_ARGS__)
#define ELOG(format, ...) LOG("ERROR", ERROR_LEVEL, format, ##__VA_ARGS__)
}
#endif