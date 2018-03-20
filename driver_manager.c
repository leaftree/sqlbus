
/**
 * driver_manager.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-16 15:00:30
 * Last Modified : 2018-03-16 15:00:30
 */

#include "dbug.h"
#include "driver_manager.h"

#define xprint(ptr) ({if(ptr==NULL){printf("[%s(%d)-%s] %s is null\n", __FILE__, __LINE__, __func__, #ptr);}})

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
int DBEnvInitialize(HENV *henv, char *catalog)
{
	if(henv == NULL || catalog == NULL)
		return(RETURN_FAILURE);

	HENV env = malloc(sizeof(environment));
	if(env == NULL) {
		return(RETURN_FAILURE);
	}

	config_t *config = malloc(sizeof(config_t));
	if(config == NULL) {
		return(RETURN_FAILURE);
	}

	if(RETURN_SUCCESS != load_config(catalog, config))
	{
		mFree(config);
		mFree(env);
		return(RETURN_FAILURE);
	}

	env->connect_counter = 0;
	env->config = config;
	*henv = env;

	return(RETURN_SUCCESS);
}

/**
 * DBEnvFinished - 释放环境句柄
 *
 * @henv: Env句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int DBEnvFinished(HENV henv)
{
	if(henv == NULL)
		return(RETURN_FAILURE);

	if(henv->config)
		unload_config(henv->config);

	mFree(henv);
		
	return(RETURN_SUCCESS);
}

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
int DBConnectInitialize(HENV henv, HDBC *hdbc)
{
	if(henv == NULL || hdbc == NULL)
		return(RETURN_FAILURE);

	*hdbc = malloc(sizeof(connection));
	if(*hdbc == NULL)
	{
		return(RETURN_FAILURE);
	}
	HDM driver = malloc(sizeof(driver_manager));
	if(driver == NULL)
	{
		mFree(*hdbc);
		return(RETURN_FAILURE);
	}
	(*hdbc)->driver = driver;
	(*hdbc)->environment = henv;

	return(RETURN_SUCCESS);
}

/**
 * DBConnectFinished - 释放数据库连接句柄
 *
 * @hdbc: 连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnectFinished(HDBC hdbc)
{
	if(hdbc == NULL)
		return(RETURN_FAILURE);

	hdbc->environment = NULL;
	mFree(hdbc);

	return(RETURN_SUCCESS);
}

/**
 * get_connection_info - 获取数据库连接信息
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
static int get_connection_info(HDBC hdbc, char *dsn, char *username, char *password, char *database)
{
	config_t *config = hdbc->environment->config;
	xprint(config);

	if(username != NULL) {
		sprintf(hdbc->username, "%s", username);
	}
	else {
		if(get_config_value(config, dsn, "username", hdbc->username) != RETURN_SUCCESS) {
			return(RETURN_FAILURE);
		}
	}

	if(password != NULL) {
		sprintf(hdbc->password, "%s", password);
	}
	else {
		if(get_config_value(config, dsn, "password", hdbc->password) != RETURN_SUCCESS) {
			return(RETURN_FAILURE);
		}
	}

	if(database != NULL) {
		sprintf(hdbc->database, "%s", database);
	}
	else {
		if(get_config_value(config, dsn, "database", hdbc->database) != RETURN_SUCCESS) {
		}
	}

	if(get_config_value(config, dsn, "driver", hdbc->catalog) != RETURN_SUCCESS) {
			DBUG_PRINT(__func__, ("dsn=%s hdbc->catalog=%s", dsn, hdbc->catalog));
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

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
int DBConnect(HDBC hdbc, char *dsn, char *username, char *password, char *database)
{
	DBUG_ENTER(__func__);
	connection *dbc = (connection*)hdbc;
	environment *env = (environment*)dbc->environment;
	config_t *config = (config_t*)env->config;

	xprint(dbc);
	xprint(env);
	xprint(config);
	xprint(hdbc->driver);
	xprint(dsn);

	if(hdbc == NULL || hdbc->environment == NULL || hdbc->environment->config == NULL || dsn == NULL)
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(RETURN_SUCCESS != get_connection_info(hdbc, dsn, username, password, database))
	{
		DBUG_PRINT(__func__, ("get_connection_info fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_PRINT(__func__, ("dsn=%s-username=%s-password=%s-database=%s", dsn, username, password, database?database:"null"));

	if(RETURN_SUCCESS != load_driver(hdbc->driver, hdbc->catalog)) {
		DBUG_PRINT(__func__, ("load_driver fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(RETURN_SUCCESS != SQLBUS_CONNECT_INIT(hdbc))
	{
		DBUG_PRINT(__func__, ("SQLBUS_CONNECT_INIT fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(RETURN_SUCCESS != SQLBUS_CONNECT(hdbc, username, password, database))
	{
		DBUG_PRINT(__func__, ("SQLBUS_CONNECT fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * DBDisconnect - 断开与数据库的连接，并卸载驱动
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者断开连接失败
 *  RETURN_SUCCESS: 成功断开连接
 */
int DBDisconnect(HDBC hdbc)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_DISCONNECT(hdbc))
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	unload_driver(hdbc->driver);

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBStmtInitialize(HDBC hdbc, HSTMT *hstmt)
{
	DBUG_ENTER(__func__);

	if(hdbc == NULL || hstmt == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	*hstmt = malloc(sizeof(statement));
	if(*hstmt == NULL)
	{
		DBUG_RETURN(RETURN_FAILURE);
	}
	(*hstmt)->connection = hdbc;

	if(RETURN_SUCCESS != SQLBUS_STMT_INIT(hdbc, *hstmt))
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

int DBStmtFinished(HSTMT hstmt)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_STMT_FREE(hstmt))
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}
