
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

st_log_meta logger;

int log_open(char *catalog, char *file, enum log_level level, st_log_meta *meta)
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

int log_close(st_log_meta *meta)
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

int log_write(st_log_meta *meta, enum log_level level,
		const char *file, const char *func, const int line, const char *fmt, ...)
{
	DBUG_ENTER(__func__);

	va_list ap;
	int size = 0;
	int retval = 0;
	char iso8061[64] = "";
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

	va_start(ap, fmt);
	size += vsnprintf(buffer+size, 16*1024, fmt, ap);
	va_end(ap);

	size += snprintf(buffer+size, 16*1024, "%s", "\n");

	DBUG_PRINT("make_iso8061_timestamp", ("%s", buffer));

	retval = write(meta->fd, buffer, size);
	if(retval <= 0)
	{
		int _errno = errno;
		DBUG_RETURN(_errno);
	}

	DBUG_RETURN(0);
}
