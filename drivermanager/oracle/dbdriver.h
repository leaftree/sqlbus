
/**
 * dbdriver.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-07 16:16:37
 * Last Modified : 2018-03-07 16:16:37
 */

#ifndef __DBDRIVER_H__
#define __DBDRIVER_H__

#include <stdint.h>

#define SQL_MSG_LEN		   512
#define SQL_MAX_ITEM_NUM 100
#define SQL_MAX_SQL_LEN	 3000

#define SQL_FIELD_TYPE_STRING  1
#define SQL_FIELD_TYPE_INTEGER 2
#define SQL_FIELD_TYPE_DOBULE  3
#define SQL_FIELD_TYPE_DATE    4

#define SQL_TYPE_QUERY 0
#define SQL_TYPE_NONE_QUERY 1

#define ORA_SQL_FIELD_TYPE_VCHAR2  1
#define ORA_SQL_FIELD_TYPE_NUMBER  2
#define ORA_SQL_FIELD_TYPE_INTEGER 3
#define ORA_SQL_FIELD_TYPE_FLOAT   4
#define ORA_SQL_FIELD_TYPE_STRING  5
#define ORA_SQL_FIELD_TYPE_LONG    8
#define ORA_SQL_FIELD_TYPE_VCHAR   9
#define ORA_SQL_FIELD_TYPE_ROWID  11
#define ORA_SQL_FIELD_TYPE_DATE   12
#define ORA_SQL_FIELD_TYPE_RAW    23
#define ORA_SQL_FIELD_TYPE_LRAW   24
#define ORA_SQL_FIELD_TYPE_CHAR   96

#define ORA_MAX_SQL_LEN SQL_MAX_SQL_LEN

#define ORA_SQL_MAX_ITEM_NUM  SQL_MAX_ITEM_NUM
#define ORA_COLUMN_NAME_LEN   SQL_MAX_ITEM_NUM
#define ORA_INDICATE_NAME_LEN SQL_MAX_ITEM_NUM

#define ORA_MAX_ROW_COUNT 10000

#define ORA_RESULT_NOT_FOUND (100)

/**
 * 数据库连接状态
 */
#define ORA_CONNECTION_STATUS_NOT (99)
#define ORA_CONNECTION_STATUS_YES (98)

#define ORA_SQL_EXEC_RESULT_CODE_FAILURE   (97)
#define ORA_SQL_EXEC_RESULT_CODE_SUCCESS   (96)
#define ORA_SQL_EXEC_RESULT_CODE_UNIQUE    (95)
#define ORA_SQL_EXEC_RESULT_CODE_NOT_FOUND (ORA_RESULT_NOT_FOUND)

#define RETURN_SUCCESS (+0)
#define RETURN_FAILURE (-1)
#define RETURN_INVALID (-2)

#ifdef mFree
# undef mFree
#endif
# define mFree(p) do { if(p) { free(p); p = NULL; } }while(0)

/**
 * 数据库操作句柄的类型
 * 0: 环境ENV句柄类型
 * 1: 数据库连接DBC句柄类型
 * 2: 数据库操纵STMT句柄类型
 */
#define ORA_SQL_HANDLE_ENV  (0)
#define ORA_SQL_HANDLE_DBC  (1)
#define ORA_SQL_HANDLE_STMT (2)

typedef void * DBHANDLE;

/**
 * error_info - 错误消息
 */
typedef struct error_info
{
	int ecode;
	char errstr[512];
}error_info;

/**
 * column_attr/field_attr - 表结构字段信息
 *
 * @name: 字段名
 * @type: 字段数据类型
 * @capacity: 字段长度
 */
typedef struct column_attr
{
	unsigned char name[64];
	uint8_t type;
	uint16_t capacity;
} field_attr, column_attr;

/**
 * table_info - 表信息
 *
 * @table_name: 表名称，不一定用得上
 * @fields: 字段/列数量
 * @rows: 查询到结果的记录条数
 * @field: 表结构字段信息
 */
typedef struct table_info
{
	char table_name[128];
	uint16_t fields;
	field_attr *field;
} table_info;

/**
 * environment/HENV
 */
typedef struct environment
{
	error_info *error;
}environment, *HENV;

/**
 * connection/HDBC - DB连接句柄
 */
typedef struct connection
{
	int port;
	char username[64];
	char password[64];
	char hostname[64];
	char database[64];
	int connection;
	error_info *error;
}connection, *HDBC;

/**
 * statment/HSTMT - DB操纵句柄
 */
typedef struct statement
{
	char *statement;
	int statement_type;
	int status; /* 应该要记录当前的执行状态 */
	int max_row_count;
	int row_cur_pos;
	int field_cur_pos;
	int result_code;
	connection *hdbc;
	table_info *table;
	error_info *error;
	int row_count;
	int table_size;
	int map_size;
	void *result;
}statement, *HSTMT;

__BEGIN_DECLS

/**
 * DBConnectInitialize - 连接数据库句柄初始化
 *
 * @hdbc: 句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_FAILURE: 分配内存空间失败
 *  RETURN_SUCCESS: 初始化成功
 */
int DBConnectInitialize(HDBC *hdbc);

/**
 * DBConnect - 数据库连接
 *
 * @hdbc: 连接句柄
 * @username: 数据库用户
 * @password: 数据库用户密码
 * @hostname: 数据库通讯地址
 * @database: 数据库实例
 * @port    : 数据库监听端口
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_FAILURE: 连接登录失败
 *  RETURN_SUCCESS: 登录数据库成功
 */
int DBConnect(HDBC hdbc, char *username, char *password, char *hostname, char *database, int port);

/**
 * DBDisconnect - 断开数据库连接
 *
 * @hdbc: 连接句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_FAILURE: 断开连接失败
 *  RETURN_SUCCESS: 断开连接成功
 */
int DBDisconnect(HDBC hdbc);

/**
 * DBStmtInitialize - 数据库操纵句柄初始化
 *
 * @hdbc: 数据库连接句柄
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_FAILURE: 内存申请失败
 *  RETURN_SUCCESS: 句柄初始化成功
 */
int DBStmtInitialize(HDBC hdbc, HSTMT *hstmt);

/**
 * DBStmtFinalize - 数据库操纵句柄使用结束，释放资源
 *
 * @hstmt: 操纵句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
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
 *  RETURN_INVALID: 参数无效
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
 *  ORA_RESULT_NOT_FOUND: 找不到数据
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
 *  ORA_RESULT_NOT_FOUND: 找不到数据
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
 *  ORA_RESULT_NOT_FOUND: 找不到数据
 */
int DBGetFieldValueIdx(HSTMT hstmt, int row, int field, char *value);

/**
 * DBGetErrorMessage - 获取错误信息
 *
 * @handle: 句柄，真实含义由type来决定
 * @type: 表示@handle的类型，其取值由数据库操作句柄的类型决定
 * @buffer: 用于存储错误信息的空间
 * @capcacity: @buffer的空间大小
 * @buffer_length: 信息串的长度
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者获取信息错误，或者无错误信息
 *  RETURN_SUCCESS: 成功获取到错误信息
 */
int DBGetErrorMessage(DBHANDLE handle, int type, char *buffer, int capacity, int *buffer_length);

/**
 * DBGetConnectionStatus - 获取数据库连接状态
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 */
int DBGetConnectionStatus(HDBC hdbc, int *status);

/**
 * DBGetExecuteResultCode - 获取SQL语句执行结果
 *
 * @hstmt: 数据库操纵句柄
 * @rcode: 结果代码
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 */
int DBGetExecuteResultCode(HSTMT hstmt, int *rcode);

__END_DECLS

#endif /* __DBDRIVER_H__ */
