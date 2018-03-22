
/**
 * log.cpp
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-10 20:43:36
 * Last Modified : 2018-03-10 20:43:36
 */

#include <string.h>
#include "log.h"
#include "util.h"
#include "dbug.h"

int log_open(char *catalog, char *file, enum log_level level, sqlbus_log_t *meta)
{
	DBUG_ENTER(__func__);

	char file_name[PATH_MAX] = "";
	if(catalog == NULL || file == NULL || meta == NULL)
		DBUG_RETURN(EINVAL);
	if(level < LOG_ERR || level > LOG_DEBUG)
		DBUG_RETURN(EINVAL);

	meta->level = level;
	meta->catalog = strdup(catalog);
	meta->file = strdup(file);
	sprintf(file_name, "%s/%s", catalog, file);

	errno = 0;
	int fd = open(file_name, O_CREAT|O_WRONLY|O_APPEND, 0640);
	if(fd < 0) {
		int _errno = errno;
		DBUG_RETURN(_errno);
	}

	meta->fd = fd;
	DBUG_RETURN(0);
}

int log_close(sqlbus_log_t *meta)
{
	DBUG_ENTER(__func__);

	if(meta == NULL)
		DBUG_RETURN(EINVAL);

	meta->fd = -1;
	if(meta->catalog)
		free(meta->catalog);
	if(meta->file)
		free(meta->file);

	DBUG_RETURN(0);
}

int log_write(sqlbus_log_t *meta, enum log_level level,
		const char *file, const char *func, const int line, const char *fmt, ...)
{
	DBUG_ENTER(__func__);

	va_list ap;
	int size = 0;
	int retval = 0;
	char buffer[16*1024] = "";

	static const char * const errmsg[] = {
		[LOG_ERR]     = "[Error]",
		[LOG_INFO]    = "[Info ]",
		[LOG_WARNING] = "[Warn ]",
		[LOG_DEBUG]   = "[Debug]",
	};

	if(meta == NULL || fmt == NULL)
		DBUG_RETURN(EINVAL);
	if(level < LOG_ERR || level > LOG_DEBUG)
		DBUG_RETURN(EINVAL);
	if(level > meta->level)
		DBUG_RETURN(0);

	size += make_iso8061_timestamp(buffer);
	size += snprintf(buffer+size, 16*1024, " %s ", (char*)errmsg[level]);

	if(meta->level == LOG_DEBUG)
		size += snprintf(buffer+size, 16*1024, "[%s(%d)-%s] ", file, line, func);

	va_start(ap, fmt);
	size += vsnprintf(buffer+size, 16*1024, fmt, ap);
	va_end(ap);

	size += snprintf(buffer+size, 16*1024, "%s", "\n");

	DBUG_PRINT("make_iso8061_timestamp", ("%s", buffer));

	errno = 0;
	retval = write(meta->fd, buffer, size);
	if(retval <= 0)
	{
		int _errno = errno;
		DBUG_RETURN(_errno);
	}

	DBUG_RETURN(0);
}

enum log_level log_level_string_to_type(char *level_string)
{
	if(!level_string)
		return(LOG_WARNING);

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

	return(LOG_WARNING);
}
