
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
#include "sqlbus.h"
#include "config_loader.h"
#include "driver_loader.h"
#include "driver_manager.h"

int main(int argc, char **argv)
{
	DBUG_PUSH("d:t:O");
	DBUG_ENTER(__func__);
	DBUG_PROCESS("main");

	int retval = 0;

	sqlbus_cycle_t cycle = {
		.config = NULL,
		.logger = NULL,
		.sqlbus = NULL,
		.db_type = NULL,
		.db_user = NULL,
		.db_auth = NULL,
		.database = NULL,
		.config_file = NULL,
	};

	sqlbus_env_init(&cycle, argc, argv);

	sqlbus_main(&cycle);

	sqlbus_env_exit(&cycle);

#if 0


	// ---------------------------------------------------------------

	HDBC hdbc = NULL;
	HENV henv = NULL;
	HSTMT hstmt = NULL;
	if(DBEnvInitialize(&henv, "./test.ini") != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBEnvInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnectInitialize(henv, &hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnectInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	//xprint_config_all(hdbc->environment->config);

	if(DBConnect(hdbc, "oracle", username, password, NULL) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnect", ("connect fail."));
		DBUG_RETURN(RETURN_FAILURE);
	}
	if(DBStmtInitialize(hdbc, &hstmt) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBStmtInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	char sql[] = "select * from basi_station_info";
	if(DBExecute(hstmt, sql) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBExecute", ("sql execute fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}
	DBUG_PRINT("DBExecute", ("sql execute succ"));

	int fields = 0;
	int rows = 0;
	char value[1000] = "";

	if(DBGetFieldCount(hstmt, &fields) != RETURN_SUCCESS)
	{
		printf("DBGetFieldCount fail\n");
		return 0;
	}
	if(DBGetRowCount(hstmt, &rows) != RETURN_SUCCESS)
	{
		printf("DBGetRowCount fail\n");
		return 0;
	}
	printf("fields=%d rows=%d\n", fields, rows);

	int i=0;
	int fieldsize=0;
	for(i=0; i<fields; i++)
	{
		if(DBGetFieldNameIdx(hstmt, i, value) != RETURN_SUCCESS)
		{
			printf("DBGetFeildNameIdx fail\n");
			return 0;
		}
		if(DBGetFieldLengthIdx(hstmt, i, &fieldsize) != RETURN_SUCCESS)
		{
			printf("DBGetFeildNameIdx fail\n");
			return 0;
		}
		printf("%s=%d\n", value, fieldsize);
	}

	for(i=0; i<rows; i++)
	{
		if(DBGetFieldNameIdx(hstmt, i, value) != RETURN_SUCCESS)
		{
			printf("DBGetFeildNameIdx fail\n");
			return 0;
		}
	}

	int j;
	for(i=0; i<rows; i++)
	{
		/*retval = DBGetNextRow(hstmt);
		if(retval == RETURN_FAILURE)
			return 0;*/
		printf("------------------------\n");
		for(j=0; j<fields; j++)
		{
			retval = DBGetFieldValueIdx(hstmt, i, j, value);
			if(retval == RETURN_FAILURE)
			{
				printf("error\n");
				return 0;
			}
			if(retval == SQLBUS_DATA_NOT_FOUND)
			{
				printf("error found\n");
				return 0;
			}
			printf("%s\n", value);
		}
	}

	if(DBStmtFinalize(hstmt) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBStmtFree", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBDisconnect(hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBDisconnect", ("Disconnect fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBConnectFinalize(hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnectInitialize", ("init fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	if(DBEnvFinalize(henv) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBEnvFinalize", ("release fail"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	unload_config(&conf);
#endif

	//log_close(&log);
	DBUG_RETURN(RETURN_SUCCESS);
}
