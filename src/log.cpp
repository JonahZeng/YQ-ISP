#include "log.h"
#include <stdarg.h>
#include <ctime>
#include <string.h>
#ifdef _MSC_VER
#include <Windows.h>
#else
#include <sys/time.h>
#endif

static int32_t g_log_level;
static FILE* g_log_fp = nullptr;


#ifdef _MSC_VER
#define LOG_TIME() {SYSTEMTIME system_time;\
    GetLocalTime(&system_time);\
    printf("[%d-%02d-%02d %02d:%02d:%02d:%03d] ", system_time.wYear, system_time.wMonth, system_time.wDay, system_time.wHour,\
        system_time.wMinute, system_time.wSecond, system_time.wMilliseconds);}
#define LOG_TIME_2FILE() {SYSTEMTIME system_time;\
    GetLocalTime(&system_time);\
    fprintf(g_log_fp, "[%d-%02d-%02d %02d:%02d:%02d:%03d] ", system_time.wYear, system_time.wMonth, system_time.wDay, system_time.wHour,\
        system_time.wMinute, system_time.wSecond, system_time.wMilliseconds);}
#define LOG_COLOR_LEVEL(level) { if(strcmp(level, "[info] ")==0 || strcmp(level, "[trace] ")==0 || strcmp(level, "[debug] ")==0)\
    {HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);CONSOLE_SCREEN_BUFFER_INFO csbiInfo;\
     GetConsoleScreenBufferInfo(cout_handle, &csbiInfo); WORD wOldColorAttrs = csbiInfo.wAttributes;\
     SetConsoleTextAttribute(cout_handle, FOREGROUND_GREEN);\
     DWORD bytesWritten = 0;\
     WriteFile(cout_handle, level, (DWORD)strlen(level), &bytesWritten, nullptr);\
     SetConsoleTextAttribute(cout_handle, wOldColorAttrs);\
    }\
    else if(strcmp(level, "[warn] ")==0 || strcmp(level, "[error] ")==0 || strcmp(level, "[critical] ")==0)\
    {HANDLE cout_handle = GetStdHandle(STD_OUTPUT_HANDLE);CONSOLE_SCREEN_BUFFER_INFO csbiInfo;\
     GetConsoleScreenBufferInfo(cout_handle, &csbiInfo); WORD wOldColorAttrs = csbiInfo.wAttributes;\
     SetConsoleTextAttribute(cout_handle, FOREGROUND_RED);\
     DWORD bytesWritten = 0;\
     WriteFile(cout_handle, level, (DWORD)strlen(level), &bytesWritten, nullptr);\
     SetConsoleTextAttribute(cout_handle, wOldColorAttrs);\
    }\
}

#else
#define LOG_TIME() {time_t now = time(nullptr);\
    tm *ltm = gmtime(&now);\
    timeval start_time;\
    gettimeofday(&start_time, nullptr);\
    printf("[%d-%02d-%02d %02d:%02d:%02d:%03ld] ", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour,\
        ltm->tm_min, ltm->tm_sec, start_time.tv_usec / 1000);}
#define LOG_TIME_2FILE() {time_t now = time(nullptr);\
    tm *ltm = gmtime(&now);\
    timeval start_time;\
    gettimeofday(&start_time, nullptr);\
    fprintf(g_log_fp, "[%d-%02d-%02d %02d:%02d:%02d:%03ld] ", ltm->tm_year + 1900, ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour,\
        ltm->tm_min, ltm->tm_sec, start_time.tv_usec / 1000);}
#define LOG_COLOR_LEVEL(level) { if(strcmp(level, "[info] ")==0 || strcmp(level, "[trace] ")==0 || strcmp(level, "[debug] ")==0){printf("\033[32m%s\033[0m", level);}\
    else if(strcmp(level, "[warn] ")==0 || strcmp(level, "[error] ")==0 || strcmp(level, "[critical] ")==0){printf("\033[31m%s\033[0m", level);}\
}
#endif


#define LOG_MONO_LEVEL(level) printf(level)


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

void open_log_file(const char* fn)
{
    g_log_fp = fopen(fn, "w");
}

void close_log_file()
{
    if(g_log_fp)
    {
        fclose(g_log_fp);
    }
}

FILE* get_log_fp()
{
    return g_log_fp;
}

void log_trace(const char *fmt, ...)
{
    if(g_log_level <= LOG_TRACE_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[trace] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else
        {
            LOG_TIME_2FILE();
            fprintf(g_log_fp, "[trace] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}

void log_debug(const char *fmt, ...)
{
    if(g_log_level <= LOG_DEBUG_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[debug] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else{
            LOG_TIME_2FILE();
           fprintf(g_log_fp, "[debug] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}

void log_info(const char *fmt, ...)
{
    if(g_log_level <= LOG_INFO_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[info] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else{
            LOG_TIME_2FILE();
            fprintf(g_log_fp, "[info] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}

void log_warning(const char *fmt, ...)
{
    if(g_log_level <= LOG_WARNING_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[warn] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else{
            LOG_TIME_2FILE();
            fprintf(g_log_fp, "[warn] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}

void log_error(const char *fmt, ...)
{
    if(g_log_level <= LOG_ERROR_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[error] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else{
            LOG_TIME_2FILE();
            fprintf(g_log_fp, "[error] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}

void log_critical(const char *fmt, ...)
{
    if(g_log_level <= LOG_CRITICAL_LEVEL)
    {
        if(g_log_fp == nullptr)
        {
            LOG_TIME();
            LOG_COLOR_LEVEL("[critical] ");
            va_list marker;
            va_start(marker, fmt);
            vprintf(fmt, marker);
            va_end(marker);
        }
        else{
            LOG_TIME_2FILE();
            fprintf(g_log_fp, "[critical] ");
            va_list marker;
            va_start(marker, fmt);
            vfprintf(g_log_fp, fmt, marker);
            va_end(marker);
        }
    }
}
