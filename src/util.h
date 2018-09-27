
/**
 * util.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-12 11:08:56
 * Last Modified : 2018-03-12 11:08:56
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define RETURN_SUCCESS (0)
#define RETURN_FAILURE (-1)

#define xprint(ptr) ({if(ptr==NULL){printf("[%s(%d)-%s] %s is null\n", __FILE__, __LINE__, __func__, #ptr);}})

#define min(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x < _y ? _x : _y; })
#define max(x,y) ({typeof(x) _x = (x); typeof(y) _y = (y); (void) (&_x == &_y); _x > _y ? _x : _y; })

#define clear_errno() (errno=0)

__BEGIN_DECLS

/**
 * make_iso8601_timestamp - 获取当前系统时间，并以iso8601格式输出
 *
 * @buffer: 保存iso8601格式的输出时间字符串
 */
int make_iso8061_timestamp(char *buffer);

/**
 * rtrim - 删除@str右边的空白符
 * @str: 需要删除空白符的字符串
 * @len: @str的长度
 */
void rtrim(char *str, int len);

/**
 * ltrim - 删除@str左边的空白符
 * @str: 需要删除空白符的字符串
 * @len: @str的长度
 */
void ltrim(char *str, int len);

/**
 * get_uuid - 获取UUID
 *
 * @uuid: 从系统中读取到的uuid值
 */
int get_uuid(char *uuid);

/**
 * console_printf - 打印消息到终端下
 */
void console_printf(const char *fmt, ...);

/**
 * iowinsize - 当前终端行宽
 */
int iowinsize();

/**
 * make_dir - 创建目录
 *
 * @path: 目录名称
 * @mode: 目录可读写执行权限
 */
int make_dir(const char *path, mode_t mode);

/**
 * get_inode - 获取文件的inode值
 *
 * @fd:    文件句柄
 * @inode: 文件inode值
 */
int get_inode(int fd,  ino_t *inode);

/**
 * readpid - 读取进程pid
 *
 * @file: pid文件路径
 * @pid:  pid的值
 *
 * 一般使用情况：由服务进程将其pid写入一个pid文件，然后由改api去pid文件中读取
 *
 * return value:
 *    0: 成功读取到pid值
 *   -1: 没有读取到pid的值
 */
int readpid(char *file, int *pid);

/**
 * match_prog_by_pid - 根据pid和程序名称匹配进程
 *
 * @pattern: 程序名称
 * @pid:     进程pid 
 *
 * return value:
 *    0: 不匹配
 *   -1: 匹配
 */
int match_prog_by_pid(char *pattern, int pid);

__END_DECLS

#endif /* __UTIL_H__ */
