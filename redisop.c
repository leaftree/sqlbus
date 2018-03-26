
/**
 * redisop.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-08 14:23:14
 * Last Modified : 2018-03-08 14:23:14
 */

#include "log.h"
#include "sqlbus.h"
#include "driver_manager.h"

/**
 * 从redis消息队列中操作数据类型
 */
static char L_key[] = "LPUSH";
static char R_key[] = "RPUSH";

/**
 * sqlbus_write_to_redis - 向redis写请求响应数据
 *
 * @handle:
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者写失败
 *  RETURN_SUCCESS: 写响应数据成功
 */
int sqlbus_write_to_redis(HSQLBUS handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;

	if(handle==NULL || handle->redis==NULL) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(handle->send_channel == NULL || handle->json_string == NULL)
	{
		sprintf(handle->errstr, "%s", strerror(EINVAL));
		DBUG_RETURN(RETURN_FAILURE);
	}

	reply = redisCommand(handle->redis, "%s %s %s",
			handle->priority==PRI_R?R_key:L_key, handle->send_channel, handle->json_string);
	if(reply == NULL || reply->type != REDIS_REPLY_INTEGER)
	{
		freeReplyObject(reply);
		DBUG_RETURN(RETURN_FAILURE);
	}

	freeReplyObject(reply);
	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * sqlbus_write_back_to_redis - 向redis中写回一个请求
 *
 * @handle
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者写回失败
 *  RETURN_SUCCESS: 写回成功
 */
int sqlbus_write_back_to_redis(HSQLBUS handle)
{
	redisReply *reply = NULL;

	if(handle == NULL || handle->redis == NULL) {
		return(RETURN_FAILURE);
	}

	if(handle->recv_channel == NULL || handle->json_string == NULL) {
		return(RETURN_FAILURE);
	}

	reply = redisCommand(handle->redis, "%s %s %s", L_key, handle->recv_channel, handle->json_string);
	if(reply == NULL || reply->type != REDIS_REPLY_INTEGER)
	{
		if(reply && reply->str) {
			sprintf(handle->errstr, "%s", reply->str);
		}

		freeReplyObject(reply);
		return(RETURN_FAILURE);
	}

	freeReplyObject(reply);

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_recv_from_redis - 从redis中读取请求
 *
 * @handle:
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者读取失败
 *  RETURN_SUCCESS: 成功读取到请求数据
 */
int sqlbus_recv_from_redis(HSQLBUS handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;
	const char defaultKey[] = "BLPOP";

	if(handle==NULL || handle->redis==NULL) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	reply = redisCommand(handle->redis, "%s %s %d",
			defaultKey, handle->recv_channel, handle->oper_timeout);
	if(reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2)
	{
		DBUG_PRINT("Reply", ("ret=%d %s %s %d", reply->type, defaultKey, handle->recv_channel, handle->oper_timeout));
		if(reply && reply->str) {
			sprintf(handle->errstr, "[redis client] %s", reply->str);
		}
		else if(reply && reply->type == REDIS_REPLY_NIL) {
			sprintf(handle->errstr, "[redis client] Read response timeout");
		}
		freeReplyObject(reply);
		DBUG_RETURN(RETURN_FAILURE);
	}

	handle->json_string = strdup(reply->element[1]->str);

	freeReplyObject(reply);
	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * sqlbus_check_redis_connection - 检查redis通讯连接状态
 *
 * @handle: SQLBUS句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数错误或者通讯错误
 *  RETURN_SUCCESS: 通讯正常
 */
int sqlbus_check_redis_connection(HSQLBUS handle)
{
	redisReply *reply = NULL;

	if(handle == NULL || handle->redis == NULL) {
		return(RETURN_FAILURE);
	}

	reply = redisCommand(handle->redis, "PING");
	if(reply == NULL || reply->type != REDIS_REPLY_STATUS)
	{
		if(reply && reply->str) {
			sprintf(handle->errstr, "%s", reply->str);
		}

		freeReplyObject(reply);
		return(RETURN_FAILURE);
	}

	sprintf(handle->errstr, "%s", reply->str);
	freeReplyObject(reply);

	return(RETURN_SUCCESS);
}

/**
 * redis_connection - 连接到redis服务端
 *
 * @host: redis服务端IP
 * @port: 连接端口
 * @timeo: 连接超时时间，0表示阻塞
 *
 * return value:
 *  NULL: 连接失败
 *  !(NULL): 连接成功
 */
redisContext *redis_connection(char *host, unsigned short int port, unsigned int timeo)
{
	char hostname[64] = "";
	struct timeval timeout = { timeo, 0 };
	redisContext *c = NULL;

	snprintf(hostname, sizeof(hostname), host?host:"127.0.0.1");

	if(timeo)
		c = redisConnectWithTimeout(hostname, port, timeout);
	else
		c = redisConnect(hostname, port);

	if (c == NULL )
	{
		return NULL;
	}
	else if(c->err)
	{
		redisFree(c);
		return NULL;
	}

	return c;
}

/**
 * redis_logout - 登出redis服务
 *
 * @redis: redis连接句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 断开连接成功
 */
int redis_logout(redisContext *redis)
{
	if(redis == NULL)
		return(RETURN_FAILURE);

	redisFree(redis);

	return(RETURN_SUCCESS);
}

/**
 * redis_auth - 认证redis连接
 *
 * @redis: redis连接句柄
 * @password: 认证密码
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或认证失败
 *  RETURN_SUCCESS: 认证成功或者不需要认证
 */
int redis_auth(redisContext *redis, char *password)
{
	int retval = RETURN_FAILURE;
	redisReply *reply = NULL;
	static const char ok[] = "OK";
	static const char invalid[] = "invalid password";
	static const char notset[] = "no password is set";

	if(redis && password)
	{
		reply = redisCommand(redis, "AUTH %s", password);
		if(reply)
		{
			if(reply->type == REDIS_REPLY_STATUS && !strcasecmp(reply->str, ok))
			{
				retval = RETURN_SUCCESS;
			}
			else if(reply->type == REDIS_REPLY_ERROR)
			{
				if(strstr(reply->str, invalid))
					retval = RETURN_FAILURE;
				else if(strstr(reply->str, notset))
					retval = RETURN_SUCCESS;
			}
			freeReplyObject(reply);
		}
	}
	return retval;
}

/**
 * redis_select - 选择redis数据库实例
 *
 * @redis: redis连接句柄
 * @database: 数据库实例
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或选择失败
 *  RETURN_SUCCESS: 选择成功
 */
int redis_select(redisContext *redis, int database)
{
	int retval = RETURN_FAILURE;
	redisReply *reply = NULL;

	if(redis)
	{
		reply = redisCommand(redis, "SELECT %d", database);
		if(reply)
		{
			if(reply->type == REDIS_REPLY_STATUS) {
				retval = RETURN_FAILURE;
			}
			else {
				retval = RETURN_SUCCESS;
			}
			freeReplyObject(reply);
		}
	}
	return retval;
}
