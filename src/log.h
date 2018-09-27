
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/param.h>

#define LOG_OPTION_OFF 0
#define LOG_OPTION_ON  1

#define defaultLogFileName "sqlbus.log"
#define defaultLogPathName "/tmp/sqlbus"
#define defaultLogWriteLevel LOG_WARNING

//
// Sqlbus log watcher thread name
//
#define SQLBUS_LOG_WATCHER_THREAD_NAME "log watcher"


enum log_level {
	LOG_ERR     = 0,
	LOG_WARNING = 1,
	LOG_INFO    = 2,
	LOG_DEBUG   = 3,
};

typedef struct sqlbus_log
{
	int             fd;         /* file decriptor of log file   */
	int             maxsize;    /* max size of log file bytes   */
	int             cron;       /* scheduled task to rolate log */
	int             stdio;      /* log file is stdout or stderr */
	int             level;      /* log base level               */
	int             trace;      /* write source code position to log file */
	int             feed;       /* add a space line after every log line  */
	char            *file;      /* log name                   */
	char            *catalog;   /* path of log                */
	ino_t           inode;      /* log file fd inode value    */
	pthread_mutex_t mutex;      /* mutex lock when try to change log handler */
} sqlbus_log_t;

int log_open(char *catalog, char *file, sqlbus_log_t *meta);
int log_close(sqlbus_log_t *meta);
int reopen_log_file(sqlbus_log_t *meta);

int log_write(sqlbus_log_t *meta, enum log_level level,
		const char *file, const char *func, const int line, const char *fmt, ...);

enum log_level log_level_string_to_type(char *level_string);

#define LOG_INFO(meta, ...)  log_write(meta, LOG_INFO,    __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_WARN(meta, ...)  log_write(meta, LOG_WARNING, __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_DEBUG(meta, ...) log_write(meta, LOG_DEBUG,   __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define LOG_ERROR(meta, ...) log_write(meta, LOG_ERR,     __FILE__, __func__, __LINE__, ##__VA_ARGS__)

int log_write_stdout(const char *fmt, ...);
int log_write_stderr(const char *fmt, ...);

/**
 * log_parse_crontime - Parse the format of cron job time
 *
 * @cron: cron time
 *
 * return value:
 *   The timestamp of distance to cron work base 00:00:00, default time is zero
 */
int log_parse_crontime(char *cron);

/**
 * log_parse_maxsize - Parse the format of max log file size
 *
 * @maxsize: MaxSize of the log file, such as: 20Mb
 * The format is support Gb/Mb/Kb/B
 * 
 * return value:
 *   The max byte size of the log file can reach, if @maxsize is in a wrong
 *   format, it will return the default size valued: 20*1024*1024
 */
int log_parse_maxsize(char *maxsize);

//
// log_op_watcher_loop - log operations watcher loop
//
// @argv: global log handler
//
// @log_op_watcher_loop do two things:
//   1. watch the log file deleted
//   2. watch the log file need to be backed up
//
// return value:
//   NULL:
void *log_op_watcher_loop(void *argv);

#endif /* __LOG_H__ */
