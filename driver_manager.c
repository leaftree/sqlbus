
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
 * DBEnvFinalize - 释放环境句柄
 *
 * @henv: Env句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int DBEnvFinalize(HENV henv)
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

	error_info_t *error = malloc(sizeof(error_info_t));
	if(error == NULL)
	{
		mFree(*hdbc);
		mFree(driver);
		return(RETURN_FAILURE);
	}

	(*hdbc)->error = error;
	(*hdbc)->driver = driver;
	(*hdbc)->environment = henv;
	(*hdbc)->conn_status = SQLBUS_DB_CONNECTION_NOT;

	return(RETURN_SUCCESS);
}

/**
 * DBConnectFinalize - 释放数据库连接句柄
 *
 * @hdbc: 连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int DBConnectFinalize(HDBC hdbc)
{
	if(hdbc == NULL)
		return(RETURN_FAILURE);

	hdbc->environment = NULL;
	mFree(hdbc->error);
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

	if(hdbc == NULL || hdbc->environment == NULL || hdbc->environment->config == NULL || dsn == NULL)
	{
		DBUG_PRINT(__func__, ("error parameter"));
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
		hdbc->conn_status = SQLBUS_DB_CONNECTION_NOT;
		DBUG_RETURN(RETURN_FAILURE);
	}
	hdbc->conn_status = SQLBUS_DB_CONNECTION_YES;

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
	hdbc->conn_status = SQLBUS_DB_CONNECTION_NOT;

	unload_driver(hdbc->driver);

	DBUG_RETURN(RETURN_SUCCESS);
}

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

	(*hstmt)->error = malloc(sizeof(error_info_t));
	if((*hstmt)->error == NULL)
	{
		mFree(*hstmt);
		return(RETURN_FAILURE);
	}

	(*hstmt)->connection = hdbc;
	(*hstmt)->result_code = SQLBUS_DB_EXEC_RESULT_FAIL;

	if(RETURN_SUCCESS != SQLBUS_STMT_INIT(hdbc, *hstmt))
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * DBStmtFinalize - 数据库操纵结束，释放资源
 *
 * @hstmt: 操作句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者释放失败
 *  RETURN_SUCCESS: 释放成功
 */
int DBStmtFinalize(HSTMT hstmt)
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
int DBExecute(HSTMT hstmt, char *statement)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL || statement == NULL) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(RETURN_SUCCESS != SQLBUS_EXECUTE(hstmt, statement))
	{
		DBGetErrorMessage(hstmt, SQLBUS_HANDLE_STMT);
		DBGetConnectionStatus(hstmt->connection);
		hstmt->result_code = SQLBUS_DB_EXEC_RESULT_FAIL;
		DBUG_RETURN(RETURN_FAILURE);
	}
	hstmt->result_code = SQLBUS_DB_EXEC_RESULT_SUCC;

	DBUG_RETURN(RETURN_SUCCESS);
}

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
int DBGetFieldCount(HSTMT hstmt, int *counter)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL || counter == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_GET_FIELD_COUNT(hstmt, counter))
	{
		DBUG_PRINT(__func__, ("SQLBUS_GET_FIELD_COUNT 失败"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
int DBGetRowCount(HSTMT hstmt, int *counter)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL || counter == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_GET_ROW_COUNT(hstmt, counter))
	{
		DBUG_PRINT(__func__, ("SQLBUS_GET_ROW_COUNT 失败"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
int DBGetFieldNameIdx(HSTMT hstmt, int index, char *value)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL || value == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_GET_FIELD_NAME(hstmt, index, value))
	{
		DBUG_PRINT(__func__, ("SQLBUS_GET_FIELD_NAME 失败"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
int DBGetFieldLengthIdx(HSTMT hstmt, int index, int *length)
{
	DBUG_ENTER(__func__);

	if(hstmt == NULL || length == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	if(RETURN_SUCCESS != SQLBUS_GET_FIELD_LENGTH(hstmt, index, length))
	{
		DBUG_PRINT(__func__, ("SQLBUS_GET_FIELD_LENGTH 失败"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * DBGetNextRow - 移动指示位置，获取下一行记录
 *
 * @hstmt: 数据库操纵句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetNextRow(HSTMT hstmt)
{
	DBUG_ENTER(__func__);

	int retval = 0;

	if(hstmt == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	retval = SQLBUS_FETCH_NEXT_ROW(hstmt);
	if(retval == RETURN_FAILURE) {
		DBUG_RETURN(RETURN_FAILURE);
	}
	else if(retval == RETURN_SUCCESS) {
		DBUG_RETURN(RETURN_SUCCESS);
	}
	else if(retval == SQLBUS_DB_DATA_NOT_FOUND) {
		DBUG_RETURN(SQLBUS_DB_DATA_NOT_FOUND);
	}

	DBUG_RETURN(RETURN_FAILURE);
}

/**
 * DBGetFieldValue - 获取当前行的字段值
 *
 * @hstmt: 数据库操纵句柄
 * @value: 字段值
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValue(HSTMT hstmt, char *value)
{
	DBUG_ENTER(__func__);

	int retval = 0;

	if(hstmt == NULL || value == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	retval = SQLBUS_GET_FIELD_VALUE(hstmt, value);
	if(retval == RETURN_FAILURE)
	{
		DBUG_RETURN(RETURN_FAILURE);
	}
	else if(retval == RETURN_SUCCESS)
	{
		DBUG_RETURN(RETURN_SUCCESS);
	}
	else if(retval == SQLBUS_DB_DATA_NOT_FOUND)
	{
		DBUG_RETURN(SQLBUS_DB_DATA_NOT_FOUND);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
 *  SQLBUS_DB_DATA_NOT_FOUND: 找不到数据
 */
int DBGetFieldValueIdx(HSTMT hstmt, int row, int field, char *value)
{
	DBUG_ENTER(__func__);

	int retval = 0;

	if(hstmt == NULL || value == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	retval = SQLBUS_GET_FIELD_VALUE_IDX(hstmt, row, field, value);
	if(retval == RETURN_FAILURE)
	{
		DBUG_RETURN(RETURN_FAILURE);
	}
	else if(retval == RETURN_SUCCESS)
	{
		DBUG_RETURN(RETURN_SUCCESS);
	}
	else if(retval == SQLBUS_DB_DATA_NOT_FOUND)
	{
		DBUG_RETURN(SQLBUS_DB_DATA_NOT_FOUND);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
int DBGetErrorMessage(HDMHANDLE handle, int type)
{
	DBUG_ENTER(__func__);

	if(handle == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	HDBC hdbc = NULL;
	HSTMT hstmt = NULL;

	int capacity = 0;
	char *buffer = NULL;
	int *buffer_length = NULL;

	switch(type)
	{
		case SQLBUS_HANDLE_DBC:
			hdbc = (HDBC)handle;
			if(hdbc->error == NULL)
				DBUG_RETURN(RETURN_FAILURE);
			capacity = hdbc->error->capacity;
			buffer = hdbc->error->errstr;
			buffer_length = &hdbc->error->msg_len;

			if(RETURN_SUCCESS != SQLBUS_GET_DBC_ERROR_MESSAGE(hdbc, buffer, capacity, buffer_length))
			{
				DBUG_PRINT(__func__, ("SQLBUS_GET_ERROR_MESSAGE 失败"));
				DBUG_RETURN(RETURN_FAILURE);
			}
			break;

		case SQLBUS_HANDLE_STMT:
			hstmt = (HSTMT)handle;
			if(hstmt->error == NULL)
				DBUG_RETURN(RETURN_FAILURE);
			capacity = hstmt->error->capacity;
			buffer = hstmt->error->errstr;
			buffer_length = &hstmt->error->msg_len;

			if(RETURN_SUCCESS != SQLBUS_GET_STMT_ERROR_MESSAGE(hstmt, buffer, capacity, buffer_length))
			{
				DBUG_PRINT(__func__, ("SQLBUS_GET_ERROR_MESSAGE 失败"));
				DBUG_RETURN(RETURN_FAILURE);
			}
			break;

		default:
			DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_PRINT("SQLBUS_GET_ERROR_MESSAGE", ("%s", buffer));

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * DBGetConnectionStatus - 获取数据库连接状态
 *
 * @hdbc: 数据库连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 获取失败
 *  RETURN_SUCCESS: 获取成功
 */
int DBGetConnectionStatus(HDBC hdbc)
{
	if(hdbc == NULL)
		return(RETURN_FAILURE);

	if(SQLBUS_GET_CONNECTION_STATUS(hdbc) != RETURN_SUCCESS)
		return(RETURN_FAILURE);

	return(RETURN_SUCCESS);
}
