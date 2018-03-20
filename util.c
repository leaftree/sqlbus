
/**
 * util.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-12 11:00:12
 * Last Modified : 2018-03-12 11:00:12
 */

#define __USE_BSD
#define __USE_MISC
#include <time.h>
#include <sys/time.h>
#include "util.h"

int make_iso8061_timestamp(char *buffer)
{
	if(buffer == NULL)
		return(0);

	struct tm mtm;
	struct timeval timev;
	struct timezone timez;

	timez.tz_minuteswest = 0;
	timez.tz_dsttime = 8;

	if(gettimeofday(&timev, &timez))
		return(0);

	if(localtime_r(&timev.tv_sec, &mtm) == NULL)
		return(0);

	return sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%06lu",
			mtm.tm_year+1900, mtm.tm_mon+1, mtm.tm_mday,
			mtm.tm_hour, mtm.tm_min, mtm.tm_sec,
			timev.tv_usec);
}

void rtrim(char *str, int len)
{
	char *end = str+len;

	while(end>=str && (*end==0x20||*end==0x0||*end==0x0a||*end==0x0d))
		*end-- = 0;
}

void ltrim(char *str, int len)
{
	int size = len;
	char *start = str;

	str[len] = 0;

	while(size && *start==0x20) size--,start++;

	if(size==len) return;

	while(size-- && *start) *str++ = *start++;

	while(*str) *str++ = 0;
}
