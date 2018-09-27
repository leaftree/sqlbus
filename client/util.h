
/**
 * util.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-11 14:24:00
 * Last Modified : 2018-04-11 14:24:00
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include "hisqlbus.h"

__BEGIN_DECLS

int getUuid(char **uuid);
int getTimestamp(unsigned int *ct);
int getAppName(char **name);
int getPid(unsigned int *pid);
int marshallJson(sqlbus *bus, char **requestString);
int unMarshallJson(sqlbus *bus, char *responseString);
int releaseJsonSource(void *data);
int getFieldName(void *data, int idx, char **name);
int getFieldIdx(void *data, char *fname, int *idx);
int getRowValue(void *data, int ridx, int cidx, char **value);

__END_DECLS

#endif /* __UTIL_H__ */
