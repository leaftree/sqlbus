
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
#include <unistd.h>
#include <stdbool.h>

#include "log.h"
#include "config_loader.h"
#include "driver_loader.h"
#include "driver_manager.h"

#include "../third/cJSON/cJSON.h"
#include "../third/hiredis/hiredis.h"

#ifndef PACKAGE_NAME
# define PACKAGE_NAME "SQLBUS"
#endif

extern int gEnvExit;

//
// Sqlbus master process name
//
#define SQLBUS_MASTER_NAME "sqlbus master"

//
// Sqlbus work process name
//
#define SQLBUS_WORK_NAME "sqlbus work"

//
// Master sqlbus process title, use for `ps`
//
#define SQLBUS_MASTER_TITLE "sqlbus: master server"

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
// Redis操作默认超时时间
//
#define defaultOperTimeOut 3

//
// 回应请求数据的有效时间
//
#define defaultExpireTime 600

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

#define defaultServicePidFile "/var/run/sqlbus.pid"

//
// Request handler
//
typedef struct sqlbus_handle {
	cJSON *root;
	cJSON *rset;
	redisContext *redis;
	int sync;
	int priority;
	int timestamp;
	int oper_timeout;
	int pid;
	char *app;
	char *type;
	char *uuid;
	char *send_channel;
	char *recv_channel;
	char *request_string;
	char *response_string;
	struct {
		int type;
		struct {
			char *update;
			char *insert;
		}ui;
		char *data;
	}statement;
	char errstr[512];
} sqlbus_handle, sqlbus_handle_t, *HSQLBUS;

//
// Global data structure
//
typedef struct sqlbus_cycle
{
	config_t *config;
	sqlbus_log_t *logger;
	sqlbus_handle_t *sqlbus;

	struct {
		int debug;
		int daemonize;
		char *pid_file;
		char *config_file;
	}envs;

	struct {
		char *section; // TODO

		int port;
		int ctimeo; // idle timeout of connection
		int rtimeo; // idle timeout of response result
		char *host;
		char *user;
		char *auth;
		char *database;
	} memcache;

	struct {
		char *section; // TODO

		HENV henv;
		HDBC hdbc;
		HSTMT hstmt;

		int port;
		char *type;
		char *host;
		char *user;
		char *auth;
		char *database;
	}db;
} sqlbus_cycle, sqlbus_cycle_t;

__BEGIN_DECLS

/**
 * sqlbus_create_pid_file - 创建pid文件
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE:
 *  RETURN_SUCCESS:
 */
int sqlbus_create_pid_file(sqlbus_cycle_t *cycle);

/**
 * sqlbus_register_signal - 注册信号处理
 */
int sqlbus_register_signal();

/**
 * sqlbus_main_entry - SQLBUS处理入口
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS:
 */
int sqlbus_main_entry(sqlbus_cycle_t *cycle);

/**
 * sqlbus_config_init - 加载sqlbus配置
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 加载成功
 *  RETURN_SUCCESS: 加载失败
 */
int sqlbus_config_init(sqlbus_cycle_t *cycle);

/**
 * sqlbus_env_init - 初始化sqlbus运行环境
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者初始化资源错误
 *  RETURN_SUCCESS: 初始化成功
 */
int sqlbus_env_init(sqlbus_cycle_t *cycle);

/**
 * sqlbus_env_exit - 退出SQLBUS服务
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_env_exit(sqlbus_cycle_t *cycle);

/**
 * sqlbus_parse_request - 解析请求数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者解析请求失败，或者缺失关键信息
 *  RETURN_SUCCESS: 解析请求成功
 */
int sqlbus_parse_request(sqlbus_cycle_t *cycle);

/**
 * sqlbus_generate_response - 创建sqlbus请求回应数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * @sqlbus_generate_response会根据数据库操纵结果失败回应消息
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 回应数据创建成功
 */
int sqlbus_generate_response(sqlbus_cycle_t *cycle);

/**
 * sqlbus_parse_env_cmd_args - 解析命令行参数
 *
 * @cycle: sqlbus主体循环接口
 * @argc: 命令行参数数量
 * @argv: 命令行参数
 *
 * return value:
 *  RETURN_FAILURE: 解析失败
 *  RETURN_SUCCESS: 解析成功
 */
int sqlbus_parse_env_cmd_args(sqlbus_cycle_t *cycle, int argc, const char *const argv[]);

/**
 * sqlbus_serviced_status_check - 检查sqlbus是否已经启动
 *
 * 如果服务已经启动，则会退出当前操作，避免影响已启动的服务
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 服务还没有启动
 *  RETURN_SUCCESS: 服务已经启动过
 */
int sqlbus_serviced_status_check(sqlbus_cycle_t *cycle);

int sqlbus_start_log_wathcer_thread(sqlbus_cycle_t *cycle);

__END_DECLS

#endif /* __SQLBUS_H__ */
