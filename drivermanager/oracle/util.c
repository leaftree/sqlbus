
/**
 * util.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-07 13:18:17
 * Last Modified : 2018-03-07 13:18:17
 */

#include "util.h"

void rtrim(unsigned char *str, int len)
{
	unsigned char *end = str+len;

	while(end>=str && (*end==0x20||*end==0x0))
		*end-- = 0;
}

void ltrim(unsigned char *str, int len)
{
	int size = len;
	unsigned char *start = str;

	str[len] = 0;

	while(size && *start==0x20) size--,start++;

	if(size==len) return;

	while(size-- && *start) *str++ = *start++;

	while(*str) *str++ = 0;
}
