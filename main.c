
/**
 * a.cpp
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-10 18:57:59
 * Last Modified : 2018-03-10 18:57:59
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "log.h"
#include "util.h"
#include "dbug.h"
#include "config_loader.h"
#include "driver_loader.h"

int main(int argc, char **argv)
{
	DBUG_PUSH("d:t:O");
	DBUG_ENTER(__func__);
	DBUG_PROCESS("main");

	int retval = 0;
	st_log_meta log;

	config_t conf;
	memset(&conf, 0x0, sizeof(config_t));

	/*
	if((retval=log_open(".", "a.txt", LOG_DEBUG, &log)) != 0)
	{
		DBUG_PRINT(__func__, ("log_open fail. %s", strerror(retval)));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if((retval=LOG_ERROR(&log, "%s", strerror(11)))!=0)
	{
		DBUG_PRINT(__func__, ("log_write fail. %s", strerror(retval)));
		DBUG_RETURN(RETURN_FAILURE);
	}
	*/

	load_config("test.ini", &conf);

	driver_manager driver;
	char driver_name[100] = "";
	char username[64] = "";
	char password[64] = "";
	if(get_config_value(&conf, "Oracle", "Driver", driver_name)!=RETURN_SUCCESS)
	{
		DBUG_PRINT(__func__, ("get_config_value[Driver] fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}
	if(get_config_value(&conf, "Oracle", "username", username)!=RETURN_SUCCESS)
	{
		DBUG_PRINT(__func__, ("get_config_value[UserName] fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}
	if(get_config_value(&conf, "Oracle", "password", password)!=RETURN_SUCCESS)
	{
		DBUG_PRINT(__func__, ("get_config_value[PassWord] fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(load_driver(&driver, driver_name) == RETURN_FAILURE)
	{
		DBUG_PRINT("load_driver", ("%s", driver.errstr));
		DBUG_RETURN(RETURN_FAILURE);
	}

	void *hdbc = NULL;
	if(DBConnectInitialize(henv, &hdbc) != RETURN_SUCCESS)
	//if(driver.func[DB_CONNECTINITIALIZE].func(&hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnectInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnect(hdbc, "oracle", username, password, NULL) != RETURN_SUCCESS)
	//if(driver.func[DB_CONNECT].func(hdbc, "", username, password, NULL) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnect", ("connect fail."));
		DBUG_RETURN(RETURN_FAILURE);
	}
	//if(DBDisconnect(&hdbc) != RETURN_SUCCESS)
	if(driver.func[DB_DISCONNECT].func(hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBDisconnect", ("Disconnect fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	//if(DBConnectFinished(&hdbc) != RETURN_SUCCESS)
	if(driver.func[DB_CONNECTFINISHED].func(hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnectInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	unload_config(&conf);

	//log_close(&log);
	DBUG_RETURN(RETURN_SUCCESS);
}
