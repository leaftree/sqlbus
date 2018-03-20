
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

#define ORA_MAX_ROW_COUNT 1000

#define ORA_SQL_EXEC_RESULT_CODE_FAILURE    (-1)
#define ORA_SQL_EXEC_RESULT_CODE_SUCCESS    (+0)
#define ORA_SQL_EXEC_RESULT_CODE_NOT_FOUND (100)

#define RETURN_SUCCESS (+0)
#define RETURN_FAILURE (-1)
#define RETURN_INVALID (-2)

#ifdef mFree
# undef mFree
#endif
# define mFree(p) do { if(p) { free(p); p = NULL; } }while(0)

typedef void* DBHENV;
typedef void* DBHDBC;
typedef void* DBHSTMT;

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
	char username[64];
	char password[64];
	char database[64];
	error_info *error;
}connection, *HDBC;

/**
 * statment/HSTMT - DB操纵句柄
 */
typedef struct statement
{
	char *statement;
	int statement_type;
	int max_row_count;
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
 * DBConnectFinished - 数据库连接句柄使用结束，翻译资源
 *
 * @hdbc: 句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_SUCCESS: 释放资源成功
 */
int DBConnectFinished(HDBC hdbc);

/**
 * DBConnect - 数据库连接
 *
 * @hdbc: 连接句柄
 * @username: 数据库用户
 * @password: 数据库用户密码
 * @database: 数据库实例
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_FAILURE: 连接登录失败
 *  RETURN_SUCCESS: 登录数据库成功
 */
int DBConnect(HDBC hdbc, char *username, char *password, char *database);

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
 * DBStmtFinished - 数据库操纵句柄使用结束，释放资源
 *
 * @hstmt: 操纵句柄
 *
 * return value:
 *  RETURN_INVALID: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int DBStmtFinished(HSTMT hstmt);

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

__END_DECLS

#endif /* __DBDRIVER_H__ */
