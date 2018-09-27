
/**
 * request.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-05-11 15:26:47
 * Last Modified : 2018-05-11 15:26:47
 */

#ifndef __REQUEST_H__
#define __REQUEST_H__

__BEGIN_DECLS

/**
 * sqlbus_parse_request - 解析请求数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者解析请求失败，或者缺失关键信息
 *  RETURN_SUCCESS: 解析请求成功
 */
int sqlbus_parse_request(sqlbus_cycle_t *cycle);

/**
 * sqlbus_free_request - 释放请求数据，包括响应数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  no return value
 */
void sqlbus_free_request(sqlbus_cycle_t *cycle);

__END_DECLS

#endif /* __REQUEST_H__ */
