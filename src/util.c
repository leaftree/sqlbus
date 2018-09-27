
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

#include "util.h"

#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

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

	return sprintf(buffer, "[%04d-%02d-%02dT%02d:%02d:%02d.%06lu]",
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
 * get_uuid - 获取UUID
 *
 * @uuid: 从系统中读取到的uuid值
 */
int get_uuid(char *uuid)
{
#define UUID_SOURCE "/proc/sys/kernel/random/uuid"
#define UUID_LENGTH (sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")-1)

	int size = 0;
	char uuid4[UUID_LENGTH+1] = "";

	int fd = open(UUID_SOURCE, O_RDONLY);
	if(fd<0) {
		return(-1);
	}

	size = read(fd, uuid4, UUID_LENGTH);
	if(size != UUID_LENGTH) {
		close(fd);
		return(-1);
	}

	sprintf(uuid, "%s", uuid4);
	close(fd);

	return(0);
}

int iowinsize()
{
	struct winsize size;
	if(!isatty(fileno(stdout)))
		return 0;
	if(ioctl(fileno(stdout), TIOCGWINSZ, &size)<0)
		return 0;
	return size.ws_col;
}

/**
 * make_dir - 创建目录
 *
 * @path: 目录名称
 * @mode: 目录可读写执行权限
 */
int make_dir(const char *path, mode_t mode)
{
	if((path==NULL || *path==0x0) || (mode<0007 || mode>0777)) {
		return(EINVAL);
	}

	struct stat fStat;
	char rec[512] = "";
	char *ptr = NULL, *src = NULL, *tmp = NULL;

	mode_t m = mode!=0?mode:0755;
	tmp = src = strdup(path);

	for(ptr=strsep(&src, "/"); ptr!=NULL; ptr=strsep(&src, "/")) {
		sprintf(rec+strlen(rec), "%s/", ptr);
		if(stat(rec, &fStat)!=0) {
			if(errno==ENOENT) {
				if(mkdir(rec, m)!=0) {
					free(tmp);
					return(errno);
				}
			}
		}
	}
	free(tmp);

	chmod(path, m);
	return(0);
}

/**
 * get_inode - 获取文件的inode值
 *
 * @fd:    文件句柄
 * @inode: 文件inode值
 */
int get_inode(int fd,  ino_t *inode)
{
	struct stat fStat;
	if(fstat(fd, &fStat)!=0) {
		return(errno);
	}

	*inode = fStat.st_ino;
	return(0);
}

int readpid(char *file, int *pid)
{
	char buf[128] = "";

	if(access(file, F_OK) == 0) {
		FILE *fp = fopen(file, "r+");
		if(fp==NULL) {
			return(-1);
		}

		if(fgets(buf, sizeof(buf)-1, fp)==NULL) {
			fclose(fp);
			return(-1);
		}
		fclose(fp);

		int value = atoi(buf);
		if(value==0) {
			return(-1);
		}
		*pid = value;
		return(0);
	}

	return(-1);
}

int match_prog_by_pid(char *pattern, int pid)
{
	char file[512] = "";
	char buff[512] = "";
	sprintf(file, "/proc/%d/cmdline", pid);

	if(access(file, F_OK) != 0) {
		return(-1);
	}

	FILE *fp = fopen(file, "r");
	if(fp==NULL) {
		return(-1);
	}
	if(fgets(buff, sizeof(buff), fp)==NULL) {
		return(-1);
	}
	fclose(fp);

	if(pattern ==NULL && strstr(buff, pattern)==NULL) {
		return(-1);
	}

	return(0);
}

