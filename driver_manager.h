
/**
 * driver_manager.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-16 14:51:16
 * Last Modified : 2018-03-16 14:51:16
 */

#ifndef __DRIVER_MANAGER_H__
#define __DRIVER_MANAGER_H__

#include "util.h"
#include "config_loader.h"
#include "driver_loader.h"

/**
 * 数据库操作句柄的类型
 * 0: 环境ENV句柄类型，调用驱动接口时不应该使用它
 * 1: 数据库连接DBC句柄类型
 * 2: 数据库操纵STMT句柄类型
 */
#define SQLBUS_HANDLE_ENV  (0)
#define SQLBUS_HANDLE_DBC  (1)
#define SQLBUS_HANDLE_STMT (2)

#define SQLBUS_DATA_NOT_FOUND (100)

/**
 * 3种驱动句柄类型
 */
typedef void * HDMENV;
typedef void * HDMDBC;
typedef void *HDMSTMT;

typedef void *HDMHANDLE;

typedef struct error_info_t
{
	int capacity;
	int msg_len;
	char errstr[1024];
} error_info, error_info_t;

/**
 * environment/HENV
 */
typedef struct environment
{
	config_t *config;
	int connect_counter;
}environment, environment_t, *HENV;

/**
 * connection/HDBC - 数据库连接句柄
 *
 * @driver: driver manager
 * @environment: environment
 */
typedef struct connection
{
	char username[64];
	char password[64];
	char database[64];
	char catalog[512];
	HDM driver;
	HENV environment;
	HDMENV driver_env;
	HDMDBC driver_dbc;
	error_info_t *error;
}connection, connection_t, *HDBC;

/**
 * statement/HSTMT - 数据库操纵句柄
 */
typedef struct statement
{
	HDBC connection;
	HDMSTMT driver_stmt;
	error_info_t *error;
}statement, statement_t, *HSTMT;

/**
 * SQLBUS_CONNECT_INIT - 数据库连接初始化
 *
 * @hdbc: 驱动初始化的连接句柄
 */
#define SQLBUS_CONNECT_INIT(hdbc) \
	(hdbc->driver->functions[DB_CONNECT_INITIALIZE].func)(&hdbc->driver_dbc)

/**
 * SQLBUS_CONNECT - 连接数据库
 *
 * @hdbc: 驱动返回数据库连接句柄
 * @user: 数据库用户名
 * @pass: 数据库用户密码
 * @db  : 数据库实例名
 */
#define SQLBUS_CONNECT(hdbc, user, pass, db) \
	(hdbc->driver->functions[DB_CONNECT].func)(hdbc->driver_dbc, user, pass, db)

/**
 * SQLBUS_DISCONNECT - 断开数据库连接
 *
 * @hdbc: 数据库连接句柄
 */
#define SQLBUS_DISCONNECT(hdbc) \
	(hdbc->driver->functions[DB_DISCONNECT].func)(hdbc)

/**
 * SQLBUS_STMT_INIT - 初始化操纵句柄
 *
 * @hdbc: 数据库连接句柄
 * @hstmt: 数据库操纵句柄
 */
#define SQLBUS_STMT_INIT(hdbc, hstmt) \
	((hstmt)->connection->driver->functions[DB_STMT_INITIALIZE].func)(hdbc->driver_dbc, &(hstmt)->driver_stmt)

/**
 * SQLBUS_STMT_FREE - 操纵句柄资源释放
 *
 * @hstmt: 数据库操纵句柄
 */
#define SQLBUS_STMT_FREE(hstmt) \
	((hstmt)->connection->driver->functions[DB_STMT_FINALIZE].func)((hstmt)->driver_stmt)

/**
 * SQLBUS_EXECUTE - 执行SQL语句
 *
 * @hstmt: 数据库操纵句柄
 * @statement: SQL语句
 */
#define SQLBUS_EXECUTE(hstmt, statement) \
	((hstmt)->connection->driver->functions[DB_STMT_EXECUTE].func)((hstmt)->driver_stmt, statement)

#define SQLBUS_GET_FIELD_COUNT(hstmt, counter) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_FIELD_COUNT].func)((hstmt)->driver_stmt, counter)

#define SQLBUS_GET_ROW_COUNT(hstmt, counter) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_ROW_COUNT].func)((hstmt)->driver_stmt, counter)

#define SQLBUS_GET_FIELD_NAME(hstmt, index, field) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_FIELD_NAME].func)((hstmt)->driver_stmt, index, field)

#define SQLBUS_GET_FIELD_LENGTH(hstmt, index, length) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_FIELD_LENGTH].func)((hstmt)->driver_stmt, index, length)

#define SQLBUS_GET_STMT_ERROR_MESSAGE(hstmt, type, buffer, capacity, buffer_length) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_ERROR_MESSAGE].func)((hstmt)->driver_stmt, type, buffer, capacity, buffer_length)

#define SQLBUS_FETCH_NEXT_ROW(hstmt) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_NEXT_ROW].func)((hstmt)->driver_stmt)

#define SQLBUS_GET_FIELD_VALUE(hstmt, value) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_FIELD_VALUE].func)((hstmt)->driver_stmt, value)

#define SQLBUS_GET_FIELD_VALUE_IDX(hstmt, row, field, value) \
	((hstmt)->connection->driver->functions[DB_STMT_GET_FIELD_VALUE_IDX].func)((hstmt)->driver_stmt, row, field, value)

#define SQLBUS_GET_DBC_ERROR_MESSAGE(hdbc, type, buffer, capacity, buffer_length) \
	((hdbc)->driver->functions[DB_STMT_GET_ERROR_MESSAGE].func)((hdbc)->driver_dbc, type, buffer, capacity, buffer_length)

__BEGIN_DECLS

/**
 * DBEnvInitialize - 初始化数据库连接环境
 *
 * @henv: Env句柄
 * @catalog: 配置文件路径
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者加载配置文件失败
 *  RETURN_SUCCESS: 加载配置文件成功
 */
int DBEnvInitialize(HENV *henv, char *catalog);

/**
 * DBEnvFinalize - 释放环境句柄
 *
 * @henv: Env句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int DBEnvFinalize(HENV henv);

/**
 * DBConnectInitialize - 初始化数据库连接句柄
 *
 * @henv: Env句柄
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者OOM
 *  RETURN_SUCCESS: 成功
 */
int DBConnectInitialize(HENV henv, HDBC *hdbc);

/**
 * DBConnectFinalize - 释放数据库连接句柄
 *
 * @hdbc: 连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnectFinalize(HDBC hdbc);

/**
 * DBConnect - 连接数据库
 *
 * @hdbc: 连接句柄
 * @dsn: Data Source Name, same as section
 * @username: 数据库用户名
 * @password: 数据库用户密码
 * @database: 数据库实例名
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnect(HDBC hdbc, char *dsn, char *username, char *password, char *database);

/**
 * DBDisconnect - 断开与数据库的连接，并卸载驱动
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者断开连接失败
 *  RETURN_SUCCESS: 成功断开连接
 */
int DBDisconnect(HDBC hdbc);

/**
 * DBStmtInitialize - 初始化数据库操纵句柄
 *
 * @hdbc: 数据库连接句柄
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者申请资源失败(包括驱动申请资源)
 *  RETURN_SUCCESS: 初始化成功
 */
int DBStmtInitialize(HDBC hdbc, HSTMT *hstmt);

/**
 * DBStmtFinalize - 数据库操纵结束，释放资源
 *
 * @hstmt: 操作句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者释放失败
 *  RETURN_SUCCESS: 释放成功
 */
int DBStmtFinalize(HSTMT hstmt);

/**
 * DBExecute - 执行SQL语句
 *
 * @hstmt: 数据库操纵句柄
 * @statement: SQL语句
 *
 * return value:
 *  RETURN_FAILURE: SQL执行失败
 *  RETURN_SUCCESS: SQL执行成功
 */
int DBExecute(HSTMT hstmt, char *statement);

/**
 * DBGetFieldCount - 获取字段数量
 *
 * @hstmt: 数据库操纵句柄
 * @counter: 出参，字段数量
 *
 * return value:
 *  RETURN_FAILURE: 参数无效，非查询SQL语句
 *  RETURN_SUCCESS: 获取字段数量成功
 */
int DBGetFieldCount(HSTMT hstmt, int *counter);

/**
 * DBGetRowCount - 获取记录行数
 *
 * @hstmt: 数据库操纵句柄
 * @counter: 出参，记录行数
 *
 * return value:
 *  RETURN_FAILURE: 参数无效，非查询SQL语句
 *  RETURN_SUCCESS: 获取字段数量成功
 */
int DBGetRowCount(HSTMT hstmt, int *counter);

/**
 * DBGetFieldNameIdx - 根据下标获取字段名称
 *
 * @hstmt: 数据库操纵句柄
 * @index: 下标，范围[0, field_counter)
 * @value: 字段的名称
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 获取字段名称成功
 */
int DBGetFieldNameIdx(HSTMT hstmt, int index, char *value);

/**
 * DBGetFieldLengthIdx - 获取字段最大长度
 *
 * @hstmt: 数据库操纵句柄
 * @index: 下标，范围[0, field_counter)
 * @length: 字段最大长度
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 获取字段长度成功
 */
int DBGetFieldLengthIdx(HSTMT hstmt, int index, int *length);

/**
 * DBGetNextRow - 移动指示位置，获取下一行记录
 *
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DATA_NOT_FOUND: 找不到数据
 */
int DBGetNextRow(HSTMT hstmt);

/**
 * DBGetFieldValue - 获取当前行的字段值
 *
 * @hstmt: 数据库操纵句柄
 * @value: 字段值
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValue(HSTMT hstmt, char *value);

/**
 * DBGetFieldValueIdx - 根据指定的行和列读取字段值
 *
 * @hstmt: 数据库操纵句柄
 * @row: 指定行数
 * @field: 指定列数
 * @value: 字段值
 *
 * 当@row为-1时，表示从当前行获取字段值
 * 当@field为-1时，表示从当前列获取字段值
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValueIdx(HSTMT hstmt, int row, int field, char *value);

/**
 * DBGetErrorMessage - 获取错误信息
 *
 * @handle: 句柄，真实含义由type来决定
 * @type: 表示@handle的类型，其取值由数据库操作句柄的类型决定
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者获取信息错误，或者无错误信息
 *  RETURN_SUCCESS: 成功获取到错误信息
 */
int DBGetErrorMessage(HDMHANDLE handle, int type);

__END_DECLS

#endif /* __DRIVER_MANAGER_H__ */
