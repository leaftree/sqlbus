
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

int sqlbus_main(sqlbus_cycle_t *cycle)
{
	DBUG_ENTER(__func__);

	if(!cycle) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	HDBC hdbc = NULL;
	HENV henv = NULL;
	HSTMT hstmt = NULL;
	if(DBEnvInitialize(&henv, cycle->config_file) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db environment failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnectInitialize(henv, &hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db connection resource failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnect(hdbc, cycle->db_type, cycle->db_user, cycle->db_auth, cycle->database) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Connect to database failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

reconnect_redis:
	cycle->sqlbus->redis = redis_connection(NULL, 6379, 2);
	if(cycle->sqlbus->redis == NULL) {
		static int reconnect_redis_times = 0;
		reconnect_redis_times ++;
		if(reconnect_redis_times>2)
			goto sqlbus_exit;
		goto reconnect_redis;
	}

	/**
	 * Authenticate
	 */
	if(cycle->mem_auth)
	{
		if(redis_auth(cycle->sqlbus->redis, cycle->mem_auth) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Authentication failure.");
		}
	}

	while(1)
	{
		if(sqlbus_recv_from_redis(cycle->sqlbus) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Waiting redis receive request failure.%s", cycle->sqlbus->errstr);
			continue;
		}
		LOG_DEBUG(cycle->logger, "[SQLBUS] json string:%s", cycle->sqlbus->json_string);

		if(sqlbus_parse_request(cycle) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Parse request failure.");
			// free JSON
			continue;
		}

		if(DBStmtInitialize(hdbc, &hstmt) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Initialize sql statement execute info failure.");
			DBUG_RETURN(RETURN_FAILURE);
		}

		if(DBExecute(hstmt, cycle->sqlbus->sql_statement) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Execute a sql statement failure.");
		}

		// generate response
		if(cycle->sqlbus->sync == JSON_OP_TRUE)
		{
			//sqlbus_check_can_response(cycle->sqlbus);
			sqlbus_generate_response(cycle->sqlbus, hstmt);
			sqlbus_write_to_redis(cycle->sqlbus);
		}

		if(DBStmtFinalize(hstmt) != RETURN_SUCCESS)
		{
			LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
			DBUG_RETURN(RETURN_FAILURE);
		}
	}

sqlbus_exit:

	if(DBDisconnect(hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Logout database failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnectFinalize(hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database connection resource failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBEnvFinalize(henv) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database environment resource failure.");
		DBUG_RETURN(RETURN_FAILURE);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

int sqlbus_env_init(sqlbus_cycle_t *cycle, int argc, const char const *argv[])
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
	cycle->db_type = strdup("Oracle");
	cycle->db_user = strdup("fzlc50db@afc");
	cycle->db_auth = strdup("fzlc50db");
	cycle->mem_auth = strdup("123kbc,./");

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

int sqlbus_generate_response(HSQLBUS handle, HSTMT hstmt)
{
	int i, j;

	if(!handle || !hstmt)
		return(RETURN_FAILURE);

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
	 * timestamp
	 */
	leaf = cJSON_CreateNumber(time(NULL));
	cJSON_AddItemToObject(root, "timestamp", leaf);

	/**
	 * message
	 */
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
		cJSON_free(root);
		return(RETURN_FAILURE);
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
		cJSON_free(root);
		return(RETURN_FAILURE);
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
	if(handle->json_string == NULL)
		printf("%s\n", "cJSON_GetErrorPtr()");
	else
	printf("json_string:%s\n", handle->json_string);

	return(RETURN_SUCCESS);
}
