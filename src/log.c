
/**
 * log.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-10 20:43:36
 * Last Modified : 2018-03-10 20:43:36
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <pthread.h>

#include "log.h"
#include "util.h"
#include "xmalloc.h"

static int log_write_console(int no, char *buffer, int size);

int log_open(char *catalog, char *file, sqlbus_log_t *meta)
{
	char file_name[PATH_MAX] = "";

	if(catalog == NULL || file == NULL || meta == NULL) {
		return(EINVAL);
	}
	if(meta->level < LOG_ERR || meta->level > LOG_DEBUG) {
		return(EINVAL);
	}

	meta->catalog = xStrdup(catalog);
	meta->file = xStrdup(file);
	sprintf(file_name, "%s/%s", catalog, file);

	clear_errno();
	if(make_dir(meta->catalog, 0755)!=0) {
		return(errno);
	}

	clear_errno();
	int fd = open(file_name, O_CREAT|O_WRONLY|O_APPEND, 0664);
	if(fd < 0) {
		return(errno);
	}
	meta->fd = fd;

	if(get_inode(meta->fd, &meta->inode)!=0) {
		return(errno);
	}
	return(0);
}

int log_close(sqlbus_log_t *meta)
{
	if(meta == NULL)
		return(EINVAL);

	close(meta->fd);
	meta->fd = -1;

	if(meta->catalog)
		xFree(meta->catalog);
	if(meta->file)
		xFree(meta->file);

	return(0);
}

int reopen_log_file(sqlbus_log_t *meta)
{
	if(!meta) {
		return(-1);
	}

	char file[512] = "";
	struct stat fdstat;

	sprintf(file, "%s/%s", meta->catalog, meta->file);

	errno = 0;
	int fd = open(file, O_CREAT|O_WRONLY|O_APPEND, 0664);
	if(fd < 0) {
		return(errno);
	}

	fstat(fd, &fdstat);

	meta->fd = fd;
	meta->inode = fdstat.st_ino;

	return(0);
}

int log_write(sqlbus_log_t *meta, enum log_level level,
		const char *file, const char *func, const int line, const char *fmt, ...)
{
#define BUFFER_MAX 4*1024
	va_list ap;
	int size = 0;
	int retval = 0;
	char buffer[BUFFER_MAX+1] = "";

	static const char * const errmsg[] = {
		[LOG_ERR]     = "[Error]",
		[LOG_INFO]    = "[Info ]",
		[LOG_WARNING] = "[Warn ]",
		[LOG_DEBUG]   = "[Debug]",
	};

	if(meta == NULL || fmt == NULL)
		return(EINVAL);
	if(level < LOG_ERR || level > LOG_DEBUG)
		return(EINVAL);
	if(level > meta->level)
		return(0);

	size = make_iso8061_timestamp(buffer);
	size += snprintf(buffer+size, BUFFER_MAX-size, " %s ", (char*)errmsg[level]);

	// jude does open source code tracer
	if(meta->trace == LOG_OPTION_ON)
		size += snprintf(buffer+size, BUFFER_MAX-size, "[%s(%d)-%s] ", file, line, func);

  // XXX: Warning
  //
  // If ap has a member which is a long string, vsnprintf will return a value more
  // then BUFFER_MAX bytes, so that it cause a core dump bug for next operation.
  //
  // Also see manual of vsnprintf.
  //
  // So, when the object string overflow BUFFER_MAX size, @size reduced 100,
  // make some space of next operation.
  //
	va_start(ap, fmt);
	size += vsnprintf(buffer+size, BUFFER_MAX-size, fmt, ap);
	va_end(ap);

  if(size >= BUFFER_MAX - 100) {
      size = BUFFER_MAX - 100;
  }

  size += snprintf(buffer+size, BUFFER_MAX-size, "%s", "\n");

	// move point to next line
	if(meta->feed == LOG_OPTION_ON)
		size += snprintf(buffer+size, BUFFER_MAX-size, "%s", "\n");

	pthread_mutex_lock(&meta->mutex);
	errno = 0;
	retval = write(meta->fd, buffer, size);
	if(retval <= 0) {
		return(errno);
	}
	fsync(meta->fd);
	pthread_mutex_unlock(&meta->mutex);

	return(0);
}

/**
 * log_parse_crontime - Parse the format of cron job time
 *
 * @cron: cron time
 *
 * return value:
 *   The timestamp of distance to cron work base 00:00:00, default time is zero
 */
int log_parse_crontime(char *cron)
{
	int tmptime = 0, crontime = 0;
	char *ptr = cron;

	if(cron == NULL) {
		return(0);
	}

	// Hour
	if((tmptime=atoi(ptr))>=24 || tmptime<0) {
		goto fatal;
	} else {
		crontime = tmptime;
	}

	// Minute
	ptr = index(ptr, ':');
	if(ptr==NULL) {
		goto fatal;
	}
	if((tmptime=atoi(ptr))>=59 || tmptime<0) {
		goto fatal;
	} else {
		crontime = crontime*60 + tmptime;
	}

	// Second
	ptr = index(ptr, ':');
	if(ptr==NULL) {
		goto fatal;
	}
	if((tmptime=atoi(ptr))>=59 || tmptime<0) {
		goto fatal;
	} else {
		crontime = crontime*60 + tmptime;
	}

	return crontime;

fatal:
		log_write_stderr("[Log->RolateCron] config item is wrong, "
				"right is such as: roloatcron=23:59:59");
		return(0);
}

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
int log_parse_maxsize(char *maxsize)
{
#define alp_to_lower(a) (a|0x20)

	int msize = 0;
	const int defaultMaxSize = 20*1024*1024; // 20mb

	if(maxsize==NULL) {
		return(defaultMaxSize);
	}

	int i=0;
	for(i=0; i<strlen(maxsize); i++) {
		if(isdigit(maxsize[i])) {
			msize = msize*10+maxsize[i]-0x30;
		} else {
			if(alp_to_lower(maxsize[i]) == 'g') {
				msize*=(1024*1024*1024);
			} else if(alp_to_lower(maxsize[i]) == 'm') {
				msize*=(1024*1024);
			} else if(alp_to_lower(maxsize[i]) == 'k') {
				msize*=(1024);
			} else if(alp_to_lower(maxsize[i]) == 'b') {
			} else {
				log_write_stderr("[Log->MaxSize] config item is wrong, right is such as: maxsize=20mb");
				return(defaultMaxSize);
			}
			break;
		}
	}

	return(msize);
}

enum log_level log_level_string_to_type(char *level_string)
{
	if(!level_string)
		return(defaultLogWriteLevel);

	if(strcasecmp(level_string, "DEBUG") == 0)
		return(LOG_DEBUG);
	else if(strcasecmp(level_string, "WARN") == 0)
		return(LOG_WARNING);
	else if(strcasecmp(level_string, "WARNING") == 0)
		return(LOG_WARNING);
	else if(strcasecmp(level_string, "ERROR") == 0)
		return(LOG_ERR);
	else if(strcasecmp(level_string, "INFO") == 0)
		return(LOG_INFO);

	return(defaultLogWriteLevel);
}

int log_write_stdout(const char *fmt, ...)
{
	va_list ap;
	int len = 0;
	char buffer[1024] = "";

	va_start(ap, fmt);
	len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	log_write_console(STDOUT_FILENO, buffer, len);
	return(0);
}

int log_write_stderr(const char *fmt, ...)
{
	va_list ap;
	int len = 0;
	char buffer[1024] = "";

	va_start(ap, fmt);
	len = vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	log_write_console(STDERR_FILENO, buffer, len);
	return(0);
}

int log_write_console(int no, char *buffer, int size)
{
	char *color = NULL;
	char *message = NULL;
	//char success[] = "[success]";
	char success[] = "[ ✔ ]";
	//char failure[] = "[failure]";
	char failure[] = "[ ✘ ]";
	char colorRed[] = "\e[1;31m";
	char colorGrn[] = "\e[1;32m";
	char colorNor[] = "\e[0m";
	int winsize = iowinsize();
	char tmp[1024] = "";
	int stmp = 0;

	if(no == STDOUT_FILENO) {
		message = success;
		color = colorGrn;
	} else if(no == STDERR_FILENO) {
		message = failure;
		color = colorRed;
	} else {
		return(EINVAL);
	}

	if(winsize < 24 && isatty(no)) {
		printf("file no = %d\n", no);
		sleep(1);
		write(no, buffer, min(size, 24));
		fsync(no);
		return(EIO);
	}

	if(isatty(no)) {
		stmp = sprintf(tmp, "%s%s%s ", color, message, colorNor);
	} else {
		stmp = sprintf(tmp, "%s ", message);
	}

	write(no, tmp, stmp);
	write(no, buffer, size);
	write(no, "\n", 1);
	fsync(no);

	return(0);
}
