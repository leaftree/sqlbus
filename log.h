
/**
 * log.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-10 20:43:36
 * Last Modified : 2018-03-10 20:43:36
 */

#ifndef __LOG_H__
#define __LOG_H__ 1

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/param.h>

#define TRACE_OFF 0
#define TRACE_ON  1

#define defaultLogFileName "sqlbus.log"
#define defaultLogPathName "/tmp/sqlbus"
#define defaultLogWriteLevel LOG_WARNING

enum log_level {
	LOG_ERR     = 0,
	LOG_WARNING = 1,
	LOG_INFO    = 2,
	LOG_DEBUG   = 3,
};

typedef struct sqlbus_log
{
	char *catalog; /* path of log                */
	char *file;    /* log name                   */
	int fd;        /* file decriptor of log file */
	int level;     /* log base level             */
	int trace;     /* write source code position to log file */
} sqlbus_log_t;

//extern sqlbus_log_t logger;

int log_open(char *catalog, char *file, enum log_level level, sqlbus_log_t *meta);
int log_close(sqlbus_log_t *meta);

int log_write(sqlbus_log_t *meta, enum log_level level,
		const char *file, const char *func, const int line, const char *fmt, ...);

enum log_level log_level_string_to_type(char *level_string);

#define LOG_INFO(meta, ...)  log_write(meta, LOG_INFO,    __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(meta, ...)  log_write(meta, LOG_WARNING, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(meta, ...) log_write(meta, LOG_DEBUG,   __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(meta, ...) log_write(meta, LOG_ERR,     __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#endif /* __LOG_H__ */
