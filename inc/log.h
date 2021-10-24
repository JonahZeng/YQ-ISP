#pragma once

#include <stdint.h>

#define LOG_CRITICAL_LEVEL	5
#define LOG_DEBUG_LEVEL 	1
#define LOG_ERROR_LEVEL	    4
#define LOG_INFO_LEVEL      2
#define LOG_NONE_LEVEL   	6
#define LOG_TRACE_LEVEL 	0
#define LOG_WARNING_LEVEL   3

int32_t get_log_level();
void set_log_level(int32_t level);
void log_critical(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_warning(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_trace(const char *fmt, ...);