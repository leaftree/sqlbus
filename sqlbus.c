
/**
 * sqlbus.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-21 15:19:22
 * Last Modified : 2018-03-21 15:19:22
 */

#include <getopt.h>
#include <unistd.h>
#include "sqlbus.h"
#include "redisop.h"

static int sqlbus_connect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle);
static int sqlbus_parse_env_cmd_args(sqlbus_cycle_t *cycle, int argc, const char *const argv[]);

/**
 * sqlbus_main_entry - SQLBUS处理入口
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS:
 */
int sqlbus_main_entry(sqlbus_cycle_t *cycle)
{
	DBUG_ENTER(__func__);

	if(!cycle) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	// 连接到redis
	while(1)
	{
		if(sqlbus_connect_to_memcache(cycle) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Connect to redis failure.");
		}
		else {
			break;
		}
		sleep(1);
	}

	// 连接到数据库
	while(1)
	{
		if(sqlbus_connect_to_database(cycle) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Connect to database failure.");
		}
		else {
			break;
		}
		sleep(1);
	}

	// 循环读取请求，执行请求，回应请求
	while(1)
	{
		// 从Redis中读取一条请求
		if(sqlbus_recv_from_redis(cycle->sqlbus) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Waiting redis receive request failure.%s", cycle->sqlbus->errstr);

			while(1)
			{
				if(sqlbus_check_redis_connection(cycle->sqlbus) != RETURN_SUCCESS) {
					sqlbus_disconnect_to_memcache(cycle);
					sqlbus_connect_to_memcache(cycle);
				}
				else {
					break;
				}
				sleep(1);
			}
			continue;
		}
		LOG_INFO(cycle->logger, "[SQLBUS] Receive request:%s", cycle->sqlbus->json_string);

		// 解析请求串，获取SQL语句，解析失败则丢弃
		if(sqlbus_parse_request(cycle) != RETURN_SUCCESS)
		{
			mFree(cycle->sqlbus->json_string);
			LOG_ERROR(cycle->logger, "[SQLBUS] Parse request failure.");
			continue;
		}

		if(DBStmtInitialize(cycle->db.hdbc, &cycle->db.hstmt) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Initialize sql statement execute info failure.");
			continue;
		}

		if(DBExecute(cycle->db.hstmt, cycle->sqlbus->sql_statement) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Execute a sql statement failure.");
			LOG_ERROR(cycle->logger, "[SQLBUS] %s.", cycle->db.hstmt->error->errstr);

			// 数据库未连接
			if(!SQLBUS_CHECK_DB_CONNECTION(cycle->db.hstmt->connection))
			{
				if(DBStmtFinalize(cycle->db.hstmt) != RETURN_SUCCESS) {
					LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
				}

				if(sqlbus_write_back_to_redis(cycle->sqlbus) != RETURN_SUCCESS)
				{
					LOG_ERROR(cycle->logger, "[SQLBUS] Write back request failure.");
					LOG_ERROR(cycle->logger, "[SQLBUS] %s.", cycle->sqlbus->errstr);
				}

				sqlbus_disconnect_to_database(cycle);

				while(1)
				{
					if(!SQLBUS_CHECK_DB_CONNECTION(cycle->db.hstmt->connection)) {
						sqlbus_connect_to_database(cycle);
					}
					else {
						break;
					}
				}
			}
		}
		mFree(cycle->sqlbus->json_string);

		// generate response
		if(cycle->sqlbus->sync == JSON_OP_TRUE) {
			sqlbus_generate_response(cycle);
			sqlbus_write_to_redis(cycle->sqlbus);
		}

		if(DBStmtFinalize(cycle->db.hstmt) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
		}
	}

	sqlbus_disconnect_to_database(cycle);
	sqlbus_disconnect_to_memcache(cycle);

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * sqlbus_env_init - 初始化sqlbus运行环境
 *
 * @cycle: sqlbus主体循环接口
 * @argc: 命令行参数数量
 * @argv: 命令行参数
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者初始化资源错误
 *  RETURN_SUCCESS: 初始化成功
 */
int sqlbus_env_init(sqlbus_cycle_t *cycle, int argc, const char *const argv[])
{
	enum log_level level = LOG_DEBUG;

	char catalog[128] = "/tmp";
	char log_file[128] = "app.log";
	char level_value[64] = "INFO";
	char config_file[128] = "/etc/sqlbus.ini";

	if(!cycle)
		return(RETURN_FAILURE);

	cycle->config = malloc(sizeof(sqlbus_config_t));
	cycle->logger = malloc(sizeof(sqlbus_log_t));
	cycle->sqlbus = malloc(sizeof(sqlbus_handle_t));

	if(cycle->config == NULL || cycle->logger == NULL || cycle->sqlbus == NULL) {
		// can not go to error_exit flag
		mFree(cycle->config);
		mFree(cycle->logger);
		mFree(cycle->sqlbus);
		console_printf("%s", "Allocate memory failure.");
		return(RETURN_FAILURE);
	}

	if(sqlbus_parse_env_cmd_args(cycle, argc, argv) != RETURN_SUCCESS) {
		console_printf("%s", "Parse environment command arguments failure.");
		goto error_exit;
	}

	if(cycle->config_file == NULL)
		cycle->config_file = strdup(config_file);

	if(load_config(cycle->config_file, cycle->config) != RETURN_SUCCESS) {
		console_printf("Load configuration file[%s] failure.[%s]", cycle->config_file, strerror(errno));
		goto error_exit;
	}

	get_config_value(cycle->config, "LOG", "CATALOG", catalog);
	get_config_value(cycle->config, "LOG", "FILENAME", log_file);
	get_config_value(cycle->config, "LOG", "LEVEL", level_value);

	level = log_level_string_to_type(level_value);

	if(log_open(catalog, log_file, level, cycle->logger) != RETURN_SUCCESS) {
		console_printf("Open log file[%s/%s] failure.", catalog, log_file);
		goto error_exit;
	}

	cycle->sqlbus->recv_channel = strdup(defaultQueue);
	cycle->sqlbus->oper_timeout = defaultOperTimeOut;

	char mem_host[64] = "";
	char mem_auth[64] = "";
	get_config_value(cycle->config, "redis", "host", mem_host);
	get_config_value(cycle->config, "redis", "Permission", mem_auth);
	cycle->memcache.host = strdup(mem_host);
	cycle->memcache.auth = strdup(mem_auth);

	char db_user[64] = "";
	char db_auth[64] = "";
	get_config_value(cycle->config, "oracle", "username", db_user);
	get_config_value(cycle->config, "oracle", "password", db_auth);
	cycle->db.type = strdup("oracle");
	cycle->db.user = strdup(db_user);
	cycle->db.auth = strdup(db_auth);

	return(RETURN_SUCCESS);

error_exit:
	unload_config(cycle->config);
	mFree(cycle->config_file);
	mFree(cycle->sqlbus->recv_channel);

	mFree(cycle->config);
	mFree(cycle->logger);
	mFree(cycle->sqlbus);
	return(RETURN_FAILURE);
}

/**
 * sqlbus_env_exit - 退出SQLBUS服务
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_env_exit(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	unload_config(cycle->config);
	log_close(cycle->logger);

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_connect_to_database - 连接到数据库
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者连接失败
 *  RETURN_SUCCESS: 连接成功
 */
int sqlbus_connect_to_database(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	if(DBEnvInitialize(&cycle->db.henv, cycle->config_file) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db environment failure.");
		return(RETURN_FAILURE);
	}

	if(DBConnectInitialize(cycle->db.henv, &cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db connection resource failure.");
		return(RETURN_FAILURE);
	}

	LOG_INFO(cycle->logger, "[SQLBUS] Database connection: Type[%s] user[%s] auth[%s] database[%s]",
			cycle->db.type, cycle->db.user, cycle->db.auth, cycle->db.database);

	if(DBConnect(cycle->db.hdbc, cycle->db.type, cycle->db.user, cycle->db.auth, cycle->db.database) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Connect to database failure.");
		if(DBGetErrorMessage(cycle->db.hdbc, SQLBUS_HANDLE_DBC) == RETURN_SUCCESS)
			LOG_ERROR(cycle->logger, "[SQLBUS] %s.", cycle->db.hdbc->error->errstr);
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_disconnect_to_database - 断开与数据库的连接
 *
 * @cycle: sqlbus主体循环接口
 *
 * @sqlbus_disconnect_to_database会释放所有与数据库通讯有关的资源
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->db.hdbc) {
		return(RETURN_FAILURE);
	}

	if(DBDisconnect(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Logout database failure.");
		if(DBGetErrorMessage(cycle->db.hdbc, SQLBUS_HANDLE_DBC) == RETURN_SUCCESS)
			LOG_ERROR(cycle->logger, "[SQLBUS] %s.", cycle->db.hdbc->error->errstr);
		//return(RETURN_FAILURE);
	}

	if(DBConnectFinalize(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database connection resource failure.");
		//return(RETURN_FAILURE);
	}

	if(DBEnvFinalize(cycle->db.henv) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database environment resource failure.");
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_connect_to_memcache - 连接到缓存
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者连接、认证失败
 *  RETURN_SUCCESS: 连接成功
 */
int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	/**
	 * Connection
	 */
	LOG_INFO(cycle->logger,
			"[SQLBUS] Memcache connection: host[%s] auth[%s] port[%d] timeout[%d]",
			cycle->memcache.host, cycle->memcache.auth, cycle->memcache.port, cycle->memcache.timeo);

	cycle->sqlbus->redis = redis_connection(cycle->memcache.host, cycle->memcache.port, cycle->memcache.timeo);
	if(cycle->sqlbus->redis == NULL) {
		LOG_ERROR(cycle->logger, "[SQLBUS] Connect to redis failure.");
		return(RETURN_FAILURE);
	}

	/**
	 * Authenticate
	 */
	if(cycle->memcache.auth) {
		if(redis_auth(cycle->sqlbus->redis, cycle->memcache.auth) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Authentication failure.");
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_disconnect_to_memcache - 断开缓存的连接
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->sqlbus->redis) {
		return(RETURN_FAILURE);
	}

	redis_logout(cycle->sqlbus->redis);
	cycle->sqlbus->redis = NULL;

	return(RETURN_SUCCESS);
}

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
int sqlbus_parse_env_cmd_args(sqlbus_cycle_t *cycle, int argc, const char *const argv[])
{
	if(!cycle || (argc && !argv)) {
		return(RETURN_FAILURE);
	}
	if(!argc) {
		return(RETURN_SUCCESS);
	}

	int opt;
	static char shortoptions[] = "c";
	static struct option longoptions[] = {
		{"config", 1, NULL, 'c'},
		{NULL, 0, NULL, 0},
	};

	while((opt=getopt_long(argc, (char*const*)argv, shortoptions, longoptions, NULL)) != -1)
	{
		switch(opt)
		{
			case 'c':
				mFree(cycle->config_file);
				cycle->config_file = strdup(optarg);
				break;

			default:
				return(RETURN_FAILURE);
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_parse_request - 解析请求数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者解析请求失败，或者缺失关键信息
 *  RETURN_SUCCESS: 解析请求成功
 */
int sqlbus_parse_request(sqlbus_cycle_t *cycle)
{
	cJSON *root = NULL;
	cJSON *leaf = NULL;

	if(!cycle || !cycle->sqlbus || !cycle->sqlbus->json_string) {
		return(RETURN_FAILURE);
	}
	//LOG_INFO(cycle->logger, "%s:%s", "[SQLBUS] request json string-", cycle->sqlbus->json_string);

	root = cJSON_Parse(cycle->sqlbus->json_string);
	if(root == NULL)
	{
		LOG_ERROR(cycle->logger, "%s-:%s", "[SQLBUS] Parse request json string failure", cJSON_GetErrorPtr());
		return(RETURN_FAILURE);
	}

	//*******************************************************************
	// Id logo
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "ID");
	if(cJSON_IsNumber(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"ID\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->crc16 = leaf->valueint;

	//*******************************************************************
	// Client logo
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "APP");
	if(cJSON_IsString(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"APP\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->app = strdup(leaf->valuestring);

	//*******************************************************************
	// Client Pid
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "PID");
	if(cJSON_IsNumber(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"PID\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->pid = leaf->valueint;

	//*******************************************************************
	// Request type
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "TYPE");
	if(cJSON_IsString(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"TYPE\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->type = strdup(leaf->valuestring);

	//*******************************************************************
	// Synchronize type
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "SYNC");
	if(cJSON_IsBool(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"SYNC\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->sync = leaf->valueint;

	//*******************************************************************
	// Response channel
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "RCHANNEL");
	if(cJSON_IsString(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"RCHANNEL\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->send_channel = strdup(leaf->valuestring);

	//*******************************************************************
	// Timestamp
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "TIMESTAMP");
	if(cJSON_IsNumber(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"TIMESTAMP\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->timestamp = leaf->valuedouble;

	//*******************************************************************
	// SQL Statement
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "STATEMENT");
	if(cJSON_IsString(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"STATEMENT\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->sql_statement = strdup(leaf->valuestring);

	return(RETURN_SUCCESS);
}

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
int sqlbus_generate_response(sqlbus_cycle_t *cycle)
{
	int i, j;

	HSTMT hstmt = NULL;
	HSQLBUS handle = NULL;

	if(!cycle || !cycle->sqlbus || !cycle->db.hstmt)
		return(RETURN_FAILURE);

	handle = cycle->sqlbus;
	hstmt = cycle->db.hstmt;

	if(handle->json_string)
		mFree(handle->json_string);

	cJSON *root = NULL, *leaf = NULL, *array = NULL;

	root = cJSON_CreateObject();

	/**
	 * Id
	 */
	leaf = cJSON_CreateNumber(handle->crc16);
	cJSON_AddItemToObject(root, "id", leaf);

	/**
	 * App 
	 */
	leaf = cJSON_CreateString("SQLBUS-SERVER");
	cJSON_AddItemToObject(root, "app", leaf);

	/**
	 * pid
	 */
	leaf = cJSON_CreateNumber(getpid());
	cJSON_AddItemToObject(root, "pid", leaf);

	/**
	 * type
	 */
	leaf = cJSON_CreateString("response");
	cJSON_AddItemToObject(root, "type", leaf);

	/**
	 * database type
	 */
	leaf = cJSON_CreateString(cycle->db.type);
	cJSON_AddItemToObject(root, "dbtype", leaf);

	/**
	 * timestamp
	 */
	leaf = cJSON_CreateNumber(time(NULL));
	cJSON_AddItemToObject(root, "timestamp", leaf);

	/**
	 * message
	 */
	if(hstmt->result_code == SQLBUS_DB_EXEC_RESULT_FAIL)
		leaf = cJSON_CreateString(hstmt->error->errstr);
	else
		leaf = cJSON_CreateString("");
	cJSON_AddItemToObject(root, "message", leaf);

	/**
	 * errorid
	 */
	leaf = cJSON_CreateNumber(0);
	cJSON_AddItemToObject(root, "errorid", leaf);

	/**
	 * Field counter
	 */
	int fields = 0;
	if(DBGetFieldCount(hstmt, &fields) != RETURN_SUCCESS)
	{
		/*
			 cJSON_free(root);
			 return(RETURN_FAILURE);
			 */
	}
	leaf = cJSON_CreateNumber(fields);
	cJSON_AddItemToObject(root, "fields", leaf);

	char field_name[128] = "";
	array = cJSON_CreateArray();
	for(i=0; i<fields; i++)
	{
		DBGetFieldNameIdx(hstmt, i, field_name);

		leaf = cJSON_CreateString(field_name);
		cJSON_AddItemToArray(array, leaf);
	}
	cJSON_AddItemToObject(root, "field", array);

	/**
	 * row counter
	 */
	int rows = 0;
	if(DBGetRowCount(hstmt, &rows) != RETURN_SUCCESS)
	{
		/*
			 cJSON_free(root);
			 return(RETURN_FAILURE);
			 */
	}
	leaf = cJSON_CreateNumber(rows);
	cJSON_AddItemToObject(root, "rows", leaf);

	/**
	 * row result set
	 */
	char value[1000] = "";
	array = cJSON_CreateArray();
	for(i=0; i<rows; i++)
	{
		cJSON *farray = cJSON_CreateArray();
		DBGetNextRow(hstmt);
		for(j=0; j<fields; j++)
		{
			DBGetFieldValueIdx(hstmt, i, j, value);
			leaf = cJSON_CreateString(value);
			cJSON_AddItemToArray(farray, leaf);
		}
		cJSON_AddItemToArray(array, farray);
	}

	cJSON_AddItemToObject(root, "result", array);

	handle->json_string = cJSON_PrintUnformatted(root);

	cJSON_free(root);

	return(RETURN_SUCCESS);
}
