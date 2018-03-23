
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
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include "util.h"

/**
 * make_iso8601_timestamp - 获取当前系统时间，并以iso8601格式输出
 *
 * @buffer: 保存iso8601格式的输出时间字符串
 */
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

/**
 * rtrim - 删除@str右边的空白符
 * @str: 需要删除空白符的字符串
 * @len: @str的长度
 */
void rtrim(char *str, int len)
{
	char *end = str+len;

	while(end>=str && (*end==0x20||*end==0x0||*end==0x0a||*end==0x0d))
		*end-- = 0;
}

/**
 * ltrim - 删除@str左边的空白符
 * @str: 需要删除空白符的字符串
 * @len: @str的长度
 */
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

/**
 * console_printf - 打印消息到终端下
 */
void console_printf(const char *fmt, ...)
{
	int len=0;
	char msg[2048] = "";

	va_list ap;
	va_start(ap, fmt);
	len = vsnprintf(msg, 2048, fmt, ap);
	va_end(ap);

	if(len == 0)
		return;

	if(isatty(fileno(stdout)))
		fprintf(stdout, "[\e[1;31mSQLBUS\e[0m] %s\n", msg);
	else
		fprintf(stdout, "[SQLBUS] %s\n", msg);
}
