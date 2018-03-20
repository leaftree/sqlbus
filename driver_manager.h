
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
 * environment/HENV
 */
typedef struct environment environment;
typedef struct environment *HENV;
typedef struct environment
{
	config_t *config;
	int connect_counter;
}environment_t;

/**
 * connection/HDBC - DB连接句柄
 *
 * @driver: driver manager
 * @environment: environment
 */

typedef void * HDMENV;
typedef void * HDMDBC;
typedef void *HDMSTMT;

typedef struct connection connection;
typedef struct connection *HDBC;
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
}connection_t;

typedef struct statement statement;
typedef struct statement *HSTMT;
typedef struct statement
{
	HDBC connection;
	HDMSTMT driver_stmt;
} statement_t;

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
	((hstmt)->connection->driver->functions[DB_STMT_FINISHED].func)((hstmt)->driver_stmt)

/**
 * SQLBUS_EXECUTE - 执行SQL语句
 *
 * @hstmt: 数据库操纵句柄
 * @statement: SQL语句
 */
#define SQLBUS_EXECUTE(hstmt, statement) \
	((hstmt)->connection->driver->functions[DB_STMT_EXECUTE].func)((hstmt)->driver_stmt, statement)

__BEGIN_DECLS

int DBEnvInitialize(HENV *henv, char *catalog);
int DBEnvFinished(HENV henv);
int DBConnectInitialize(HENV henv, HDBC *hdbc);
int DBConnectFinished(HDBC hdbc);
int DBConnect(HDBC hdbc, char *dsn, char *username, char *password, char *database);
int DBDisconnect(HDBC hdbc);

int DBStmtInitialize(HDBC hdbc, HSTMT *hstmt);
int DBStmtFinished(HSTMT hstmt);

__END_DECLS

#endif /* __DRIVER_MANAGER_H__ */
