
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

redisContext *defaultRedisHandle = NULL;

int sqlbus_write_to_redis(HSQLBUS handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;
	static char L_key[] = "LPUSH";
	static char R_key[] = "RPUSH";

	if(handle==NULL || handle->redis==NULL)
		DBUG_RETURN(RETURN_FAILURE);

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

int sqlbus_recv_from_redis(HSQLBUS handle)
{
	DBUG_ENTER(__func__);

	redisReply *reply = NULL;
	const char defaultKey[] = "BLPOP";

	if(handle==NULL || handle->redis==NULL)
		DBUG_RETURN(RETURN_FAILURE);

	reply = redisCommand(handle->redis, "%s %s %d",
			defaultKey, handle->recv_channel, handle->oper_timeout);
	if(reply == NULL || reply->type != REDIS_REPLY_ARRAY || reply->elements != 2)
	{
		DBUG_PRINT("Reply", ("ret=%d %s %s %d", reply->type, defaultKey, handle->recv_channel, handle->oper_timeout));
		if(reply && reply->str)
			sprintf(handle->errstr, "[redis client] %s", reply->str);
		else if(reply && reply->type == REDIS_REPLY_NIL)
			sprintf(handle->errstr, "[redis client] Read response timeout");
		freeReplyObject(reply);
		DBUG_RETURN(RETURN_FAILURE);
	}

	handle->json_string = strdup(reply->element[1]->str);

	freeReplyObject(reply);
	DBUG_RETURN(RETURN_SUCCESS);
}

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

int redis_auth(redisContext *redis, char *password)
{
	int retval = -1;
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

int redis_select(redisContext *redis, int database)
{
	int retval = -1;
	redisReply *reply = NULL;

	if(redis)
	{
		reply = redisCommand(redis, "SELECT %s", database);
		if(reply)
		{
			if(reply->type == REDIS_REPLY_STATUS)
			{
				retval = 0;
			}
			else
			{
				retval = -1;
			}
			freeReplyObject(reply);
		}
	}
	return retval;
}
