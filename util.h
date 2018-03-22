
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
#include <stdlib.h>

#define RETURN_SUCCESS (0)
#define RETURN_FAILURE (-1)

#ifdef mFree
# undef mFree
#endif
# define mFree(ptr) do { if(ptr) { free(ptr); ptr = NULL; } }while(0)

#define xprint(ptr) ({if(ptr==NULL){printf("[%s(%d)-%s] %s is null\n", __FILE__, __LINE__, __func__, #ptr);}})

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

__END_DECLS

#endif /* __UTIL_H__ */
