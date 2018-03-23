
/**
 * sqlbus.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-21 15:19:22
 * Last Modified : 2018-03-21 15:19:22
 */

#include "sqlbus.h"
#include "redisop.h"

static int sqlbus_connect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle);

int sqlbus_main(sqlbus_cycle_t *cycle)
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
			//sqlbus_check_can_response(cycle->sqlbus);
			sqlbus_generate_response(cycle, cycle->db.hstmt);
			sqlbus_write_to_redis(cycle->sqlbus);
		}

		if(DBStmtFinalize(cycle->db.hstmt) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
			DBUG_RETURN(RETURN_FAILURE);
		}
	}

	sqlbus_disconnect_to_database(cycle);
	sqlbus_disconnect_to_memcache(cycle);

	DBUG_RETURN(RETURN_SUCCESS);
}

int sqlbus_env_init(sqlbus_cycle_t *cycle, int argc, const char *const argv[])
{
	enum log_level level = LOG_DEBUG;

	char catalog[64] = ".";
	char log_file[64] = "app.log";
	char level_value[64] = "DEBUG";
	char config_file[128] = "test.ini";

	if(!cycle)
		return(RETURN_FAILURE);

	//
	//test
	// 
	cycle->db.type = strdup("Oracle");
	cycle->db.user = strdup("fzlc50db@afc");
	cycle->db.auth = strdup("fzlc50db");
	cycle->memcache.auth = strdup("123kbc,./");

	cycle->config_file = strdup(config_file);
	cycle->config = malloc(sizeof(sqlbus_config_t));
	cycle->logger = malloc(sizeof(sqlbus_log_t));
	cycle->sqlbus = malloc(sizeof(sqlbus_handle_t));
	cycle->sqlbus->recv_channel = strdup(defaultQueue);
	cycle->sqlbus->oper_timeout = defaultOperTimeOut;

	if(cycle->config_file == NULL || cycle->config == NULL || cycle->logger == NULL || cycle->sqlbus == NULL) {
		printf("[SQLBUS] Allocate memory error.\n");
		mFree(cycle->config_file);
		mFree(cycle->config);
		mFree(cycle->logger);
		mFree(cycle->sqlbus);
		return(RETURN_FAILURE);
	}

	if(load_config(config_file, cycle->config) != RETURN_SUCCESS) {
		printf("[SQLBUS] Load configuration file failure.\n");
		mFree(cycle->config_file);
		mFree(cycle->config);
		mFree(cycle->logger);
		mFree(cycle->sqlbus);
		return(RETURN_FAILURE);
	}

	get_config_value(cycle->config, "LOG", "CATALOG", catalog);
	get_config_value(cycle->config, "LOG", "FILENAME", log_file);
	get_config_value(cycle->config, "LOG", "LEVEL", level_value);

	level = log_level_string_to_type(level_value);

	if(log_open(catalog, log_file, level, cycle->logger) != RETURN_SUCCESS) {
		printf("[SQLBUS] Open log file[%s/%s] failure.\n", catalog, log_file);
		unload_config(cycle->config);
		mFree(cycle->config_file);
		mFree(cycle->config);
		mFree(cycle->logger);
		mFree(cycle->sqlbus);
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

int sqlbus_env_exit(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	unload_config(cycle->config);
	log_close(cycle->logger);

	return(RETURN_SUCCESS);
}

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

static int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->db.hdbc) {
		return(RETURN_FAILURE);
	}

	if(DBDisconnect(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Logout database failure.");
		if(DBGetErrorMessage(cycle->db.hdbc, SQLBUS_HANDLE_DBC) == RETURN_SUCCESS)
			LOG_ERROR(cycle->logger, "[SQLBUS] %s.", cycle->db.hdbc->error->errstr);
		return(RETURN_FAILURE);
	}

	if(DBConnectFinalize(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database connection resource failure.");
		return(RETURN_FAILURE);
	}

	if(DBEnvFinalize(cycle->db.henv) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database environment resource failure.");
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	/**
	 * Connection
	 */
	LOG_INFO(cycle->logger,
			"[SQLBUS] Memcache connection: host[%s] port[%d] timeout[%d]",
			cycle->memcache.host, cycle->memcache.port, cycle->memcache.timeo);

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

int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->sqlbus->redis) {
		return(RETURN_FAILURE);
	}

	redis_logout(cycle->sqlbus->redis);
	cycle->sqlbus->redis = NULL;

	return(RETURN_SUCCESS);
}

int sqlbus_parse_request(sqlbus_cycle_t *cycle)
{
	cJSON *root = NULL;
	cJSON *leaf = NULL;

	if(!cycle || !cycle->sqlbus || !cycle->sqlbus->json_string) {
		return(RETURN_FAILURE);
	}

	LOG_INFO(cycle->logger, "%s:%s", "[SQLBUS] request json string-", cycle->sqlbus->json_string);

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

int sqlbus_check_can_response(HSQLBUS handle)
{
	if(!handle)
		return(RETURN_FAILURE);

	return(RETURN_SUCCESS);
}

int sqlbus_generate_response(sqlbus_cycle_t *cycle, HSTMT hstmt)
{
	int i, j;

	HSQLBUS handle = NULL;

	if(!cycle || !cycle->sqlbus || !hstmt)
		return(RETURN_FAILURE);

	handle = cycle->sqlbus;

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
