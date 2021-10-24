#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#include <sys/time.h>

static int32_t g_log_level;

void set_log_level(int32_t level)
{
    if(level >= LOG_TRACE_LEVEL && level <= LOG_NONE_LEVEL)
    {
        g_log_level = level;
    }
}

int32_t get_log_level()
{
    return g_log_level;
}

void log_trace(const char *fmt, ...)
{
    if(g_log_level <= LOG_TRACE_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [trace]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}

void log_debug(const char *fmt, ...)
{
    if(g_log_level <= LOG_DEBUG_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [debug]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}

void log_info(const char *fmt, ...)
{
    if(g_log_level <= LOG_INFO_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [info]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}

void log_warning(const char *fmt, ...)
{
    if(g_log_level <= LOG_WARNING_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [warning]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}

void log_error(const char *fmt, ...)
{
    if(g_log_level <= LOG_ERROR_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [error]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}

void log_critical(const char *fmt, ...)
{
    if(g_log_level <= LOG_CRITICAL_LEVEL)
    {
        time_t now = time(nullptr);
        tm *ltm = gmtime(&now);
        timeval start_time;
        gettimeofday(&start_time, NULL);
        printf("[%d-%d-%d %d:%d:%d:%03ld] [critical]", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour, 
            ltm->tm_min, ltm->tm_sec, start_time.tv_usec/1000);

        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}