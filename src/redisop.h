
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

/**
 * sqlbus_write_to_redis - 向redis中写请求回应
 */
int sqlbus_write_to_redis(HSQLBUS handle);

/**
 * sqlbus_recv_from_redis - 从redis中读取请求
 */
int sqlbus_recv_from_redis(HSQLBUS handle);

/**
 * sqlbus_write_back_to_redis - 向redis中写回一个请求
 *
 * 当sqlbus从redis中读出一个请求执行时，由于数据库连接断开，
 * 则将该请求写回redis中
 */
int sqlbus_write_back_to_redis(HSQLBUS handle);

/**
 * sqlbus_check_redis_connection - 检查redis连接状态
 */
int sqlbus_check_redis_connection(HSQLBUS handle);

/**
 * redis_connection - 连接redis服务
 */
redisContext *redis_connection(char *host, unsigned short int port, unsigned int timeo);

/**
 * redis_auth - 认证redis连接
 */
int redis_auth(redisContext *redis, char *password);

/**
 * redis_select - 选择redis数据库实例
 */
int redis_select(redisContext *redis, int database);

/**
 * redis_logout - 登出redis服务
 */
int redis_logout(redisContext *redis);

/**
 * redis_expire - 设置key的过期时间
 *
 * 时间精度为秒
 */
int redis_expire(redisContext *redis, const char *key, unsigned int timeo);

__END_DECLS

#endif /* __REDISOP_H__ */
