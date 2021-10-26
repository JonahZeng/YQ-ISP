#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <ctime>
#include <string.h>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/time.h>
#endif

static int32_t g_log_level;


#ifdef _MSC_VER
#define LOG_TIME(level) {SYSTEMTIME system_time;\
    GetLocalTime(&system_time);\
    printf("[%d-%02d-%02d %02d:%02d:%02d:%03d] [%s] ", system_time.wYear, system_time.wMonth, system_time.wDay, system_time.wHour,\
        system_time.wMinute, system_time.wSecond, system_time.wMilliseconds, level);}
#else
#define LOG_TIME() {time_t now = time(nullptr);\
    tm *ltm = gmtime(&now);\
    timeval start_time;\
    gettimeofday(&start_time, NULL);\
    printf("[%d-%02d-%02d %02d:%02d:%02d:%03ld] ", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour,\
        ltm->tm_min, ltm->tm_sec, start_time.tv_usec / 1000);}
#define LOG_COLOR_LEVEL(level) { if(strcmp(level, "info")==0 || strcmp(level, "trace")==0 || strcmp(level, "debug")==0){printf("[\033[32m%s\033[0m] ", level);}\
    else if(strcmp(level, "warn")==0 || strcmp(level, "error")==0 || strcmp(level, "critical")==0){printf("[\033[31m%s\033[0m] ", level);}\
}
#endif


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
        LOG_TIME();
        LOG_COLOR_LEVEL("trace");
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
        LOG_TIME();
        LOG_COLOR_LEVEL("debug");
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
        LOG_TIME();
        LOG_COLOR_LEVEL("info");
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
        LOG_TIME();
        LOG_COLOR_LEVEL("warn");
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
        LOG_TIME();
        LOG_COLOR_LEVEL("error");
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
        LOG_TIME();
        LOG_COLOR_LEVEL("critical");
        va_list marker;
        va_start(marker, fmt);
        vprintf(fmt, marker);
        va_end(marker);
    }
}