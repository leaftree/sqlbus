
/**
 * driver_manager.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-16 14:51:16
 * Last Modified : 2018-03-16 14:51:16
 */

#ifndef __DRIVER_MANAGER_H__
#define __DRIVER_MANAGER_H__

#include "util.h"
#include "config_loader.h"
#include "driver_loader.h"

/**
 * environment/HENV
 */
typedef struct environment
{
	config_t *config;
	int connect_counter;
}environment, *HENV;

/**
 * connection/HDBC - DB连接句柄
 *
 * @driver: driver manager
 * @environment: environment
 */
typedef struct connection connection;
typedef struct connection *HDBC;
struct connection
{
	char username[64];
	char password[64];
	char database[64];
	char catalog[512];
	HDM driver;
	HENV environment;
	HENV driver_env;
	HDBC driver_dbc;
};

#define DB_FUNC_CONNECT 

__BEGIN_DECLS

int DBEnvInitialize(HENV *henv, char *catalog);
int DBEnvFinished(HENV henv);

int DBConnectInitialize(HENV henv, HDBC *hdbc);

int DBConnectFinished(HDBC hdbc);

int DBConnect(HDBC hdbc, char *dsn, char *username, char *password, char *database);

int DBDisconnect(void *ptr);

__END_DECLS

#endif /* __DRIVER_MANAGER_H__ */
