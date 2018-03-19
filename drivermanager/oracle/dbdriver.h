
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

#define SQL_MSG_LEN		   512
#define SQL_MAX_ITEM_NUM 100
#define SQL_MAX_SQL_LEN	 3000

#define SQL_FIELD_TYPE_STRING  1
#define SQL_FIELD_TYPE_INTEGER 2
#define SQL_FIELD_TYPE_DOBULE  3
#define SQL_FIELD_TYPE_DATE    4

#define SQL_TYPE_QUERY 0
#define SQL_TYPE_NONE_QUERY 1

#if defined(DBO_FAIL) || defined(DBO_SUCC)
# undef DBO_FAIL
# undef DBO_SUCC
#endif
#define DBO_SUCC (+0)
#define DBO_FAIL (-1)

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

#define ORA_MAX_ROW_COUNT 10

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
	uint32_t rows;
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
typedef struct statment
{
	char *statment;
	int statment_type;
	int max_row_count;
	connection *hdbc;
	table_info *table;
	error_info *error;
}statment, *HSTMT;

__BEGIN_DECLS

/*
DBDriver *DBHEnvNew(char *db, char *user, char *password);
int DBConnection(DBDriver *driver);
int DBExecute(DBDriver *driver, char *statment);
int DBStmtFree(DBDriver *driver);
int DBCloseConnection(DBDriver *driver);
int DBReleaseHEnv(DBDriver **driver);
*/

__END_DECLS

#endif /* __DBDRIVER_H__ */
