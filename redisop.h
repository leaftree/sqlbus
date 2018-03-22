
/**
 * redisop.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-22 09:39:19
 * Last Modified : 2018-03-22 09:39:19
 */

#ifndef __REDISOP_H__
#define __REDISOP_H__

__BEGIN_DECLS

int sqlbus_write_to_redis(HSQLBUS handle);
int sqlbus_recv_from_redis(HSQLBUS handle);
redisContext *redis_connection(char *host, unsigned short int port, unsigned int timeo);
int redis_auth(redisContext *redis, char *password);

int redis_select(redisContext *redis, int database);

__END_DECLS

#endif /* __REDISOP_H__ */
