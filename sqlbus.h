
/**
 * sqlbus.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-05 13:11:44
 * Last Modified : 2018-03-05 13:11:44
 */

#ifndef __SQLBUS_H__
#define __SQLBUS_H__

#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <malloc.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include "dbug.h"
#include "cJSON.h"
#include "hiredis.h"

#include "log.h"
#include "util.h"
#include "config_loader.h"
#include "driver_loader.h"
#include "driver_manager.h"

//
// 默认Redis连接
//
extern redisContext *defaultRedisHandle;

//
//
//
#define JSON_OP_FALSE (false)
#define JSON_OP_TRUE  (true)

//
// 执行SQL时可以选择同步SYNC或异步ASYNC的方式
// 异步：将SQL发送到缓存后即刻返回
// 同步：将SQL发送到缓存后阻塞读SQL执行结果
// 建议在执行查询SQL语句时才使用同步方式
//
#define SQLBUS_ASYNC 0
#define SQLBUS_SYNC 1
#define SQLBUS_TIMEO 2
#define SQLBUS_CLIENT 4
#define SQLBUS_SERVER 8

//
// 默认发送+接收队列名称
//
#define defaultQueue "SqlBusDefaultQueue"

//
// Redis操作默认超时时间，0表示阻塞
//
#define defaultOperTimeOut 0

//
// 优先级定义：R->右 L->左
// 默认发送：R
// 正常接收：L
// 若发送时使用L进行发送，则会优先处理
//
#define PRI_R 0
#define PRI_L 1
#define defaultSendChannelPriority PRI_R
#define defaultReadChannelPriority PRI_L

//
//
//
typedef struct sqlbus_handle {
	cJSON *root;
	cJSON *rset;
	redisContext *redis;
	int ora;
	int idx;
	int sync;
	int more;
	int crc16;
	int priority;
	int timestamp;
	int oper_timeout;
	int pid;
	char *app;
	char *type;
	char *send_channel;
	char *recv_channel;
	char *json_string;
	char *sql_statement;
	char errstr[512];
} sqlbus_handle, sqlbus_handle_t, *HSQLBUS;

typedef struct sqlbus_cycle
{
	config_t *config;
	sqlbus_log_t *logger;
	sqlbus_handle_t *sqlbus;

	char *db_type;
	char *db_host;
	char *db_user;
	char *db_auth;
	char *database;
	char *mem_host;
	char *mem_user;
	char *mem_auth;
	char *mem_database;
	char *config_file;
} sqlbus_cycle, sqlbus_cycle_t;

__BEGIN_DECLS

__END_DECLS

#endif /* __SQLBUS_H__ */
