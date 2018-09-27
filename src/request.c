
/**
 * request.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-05-11 15:25:37
 * Last Modified : 2018-05-11 15:25:37
 */

#include <getopt.h>
#include <unistd.h>

#include "xmalloc.h"
#include "sqlbus.h"
#include "redisop.h"
#include "request.h"

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

	if(!cycle || !cycle->sqlbus || !cycle->sqlbus->request_string) {
		return(RETURN_FAILURE);
	}

	root = cJSON_Parse(cycle->sqlbus->request_string);
	if(root == NULL)
	{
		LOG_ERROR(cycle->logger, "%s-:%s", "[SQLBUS] Parse request json string failure", cJSON_GetErrorPtr());
		return(RETURN_FAILURE);
	}

	//*******************************************************************
	// uuid logo
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "UUID");
	if(cJSON_IsString(leaf) == JSON_OP_FALSE)
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"UUID\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}
	cycle->sqlbus->uuid = xStrdup(leaf->valuestring);

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
	cycle->sqlbus->app = xStrdup(leaf->valuestring);

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
	cycle->sqlbus->type = xStrdup(leaf->valuestring);

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
	cycle->sqlbus->sync = leaf->valueint==true?SQLBUS_SYNC:SQLBUS_ASYNC;

	//*******************************************************************
	// Response channel
	//*******************************************************************
	leaf = cJSON_GetObjectItem(root, "RCHANNEL");
	if(cJSON_IsString(leaf) == JSON_OP_TRUE) {
		cycle->sqlbus->send_channel = xStrdup(leaf->valuestring);
	}
	else
	{
		if(cycle->sqlbus->sync == SQLBUS_SYNC)
		{
			LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"RCHANNEL\" from request json string failure.");
			cJSON_free(root);
			return(RETURN_FAILURE);
		}
	}

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
	if(cJSON_IsString(leaf) == JSON_OP_TRUE)
	{
		cycle->sqlbus->statement.type = 1;
		cycle->sqlbus->statement.data = xStrdup(leaf->valuestring);
	}
	else if(cJSON_IsObject(leaf) == JSON_OP_TRUE)
	{
		cJSON *update = NULL, *insert = NULL;
		cycle->sqlbus->statement.type = 2;
		update = cJSON_GetObjectItem(leaf, "UPDATE");
		insert = cJSON_GetObjectItem(leaf, "INSERT");

		if(cJSON_IsString(update) == JSON_OP_FALSE || cJSON_IsString(insert) == JSON_OP_FALSE)
		{
			LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"STATEMENT\" from request json string failure.");
			cJSON_free(root);
			return(RETURN_FAILURE);
		}
		cycle->sqlbus->statement.ui.update = xStrdup(update->valuestring);
		cycle->sqlbus->statement.ui.insert = xStrdup(insert->valuestring);
	}
	else
	{
		LOG_ERROR(cycle->logger, "%s", "[SQLBUS] Get \"STATEMENT\" from request json string failure.");
		cJSON_free(root);
		return(RETURN_FAILURE);
	}


	if(cycle->sqlbus->statement.type == 2) {
		if(cycle->sqlbus->statement.ui.update)
			LOG_INFO(cycle->logger, "ui.update=%s", cycle->sqlbus->statement.ui.update);
		if(cycle->sqlbus->statement.ui.insert)
			LOG_INFO(cycle->logger, "ui.insert=%s", cycle->sqlbus->statement.ui.insert);
	} else {
		if(cycle->sqlbus->statement.data)
			LOG_INFO(cycle->logger, "data=%s", cycle->sqlbus->statement.data);
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_free_request - 释放请求数据，包括响应数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  no return value
 */
void sqlbus_free_request(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->sqlbus) {
		return;
	}

	xFree(cycle->sqlbus->uuid);
	xFree(cycle->sqlbus->app);
	xFree(cycle->sqlbus->type);
	xFree(cycle->sqlbus->send_channel);
	xFree(cycle->sqlbus->request_string);
	xFree(cycle->sqlbus->response_string);

	if(cycle->sqlbus->statement.type == 1) {
		xFree(cycle->sqlbus->statement.data);
	} else {
		xFree(cycle->sqlbus->statement.ui.update);
		xFree(cycle->sqlbus->statement.ui.insert);
	}

	return;
}
