
/**
 * dbservice.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-02 11:03:12
 * Last Modified : 2018-03-02 11:03:12
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <limits.h>
#include <signal.h>

#include "dbug.h"
#include "dbdriver.h"

const static int rport = 6379;

int main()
{
	int field;
	int row;
	int retval;
				char value[64] = "";
	DBUG_PUSH ("d:t:O");
	DBUG_ENTER(__func__);
	DBUG_PROCESS ("测试程序");
	prctl(PR_SET_NAME, "测试程序");

	HDBC hdbc = NULL;
	if(DBConnectInitialize(&hdbc) != RETURN_SUCCESS)
	{
		DBUG_PRINT("DBConnectInitialize", ("Create database operate handle env fail"));
		DBUG_RETURN(-1);
	}
	if(RETURN_SUCCESS != DBConnect(hdbc, "fzlc50db@afc", "fzlc50db", NULL))
	{
		DBUG_PRINT("Connect to database fail", ("%s", hdbc->error->errstr));
		DBUG_RETURN(-1);
	}

	HSTMT hstmt = NULL;
	DBStmtInitialize(hdbc, &hstmt);
	if(RETURN_SUCCESS != DBExecute(hstmt, "select * from basi_station_info"))
		//if(RETURN_SUCCESS != DBExecute(hstmt, "update basi_station_info set location_number='0' where station_id = '0200'"))
	{
		DBUG_PRINT("execute fail", ("%s", hstmt->error->errstr));
		DBUG_PRINT("ORA_SQL_EXEC_RESULT_CODE_FAILURE", ("%d", hstmt->result_code));
		DBUG_RETURN(-1);
	}

	if(RETURN_SUCCESS != DBGetFieldCount(hstmt, &field))
	{
		DBUG_PRINT("DBGetFieldCount fail", ("%s", hstmt->error->errstr));
		DBUG_RETURN(-1);
	}
	printf("field num=%d\n", field);

	char buffer[512] = "";
	int buffer_length = 0;
	int i=0;
	for(i=0; i<field; i++)
	{
		char value[64] = "";
		int size=0;
		if(RETURN_SUCCESS != DBGetFieldNameIdx(hstmt, i, value))
		{
			DBUG_PRINT("DBGetFieldNameIdx fail", ("info:%s", hstmt->error->errstr));
			DBUG_RETURN(-1);
		}
		if(RETURN_SUCCESS != DBGetFieldLengthIdx(hstmt, i, &size))
		{
			if(RETURN_SUCCESS == DBGetErrorMessage(hstmt, ORA_SQL_HANDLE_STMT, buffer, 512, &buffer_length))
			{
				DBUG_PRINT("DBGetFieldLengthIdx fail", ("info:%s", buffer));
			}
			DBUG_RETURN(-1);
		}
		DBUG_PRINT("DBGetFieldNameIdx ", ("%s-%d", value, size));
	}

	if(RETURN_SUCCESS != DBGetRowCount(hstmt, &row))
	{
		if(RETURN_SUCCESS == DBGetErrorMessage(hstmt, ORA_SQL_HANDLE_STMT, buffer, 512, &buffer_length))
		{
			DBUG_PRINT("DBGetRowCount fail", ("info:%s", buffer));
		}
		DBUG_RETURN(-1);
	}
	printf("row row=%d\n", row);

	while(1)
	{
		retval = DBGetNextRow(hstmt);
		if(retval == RETURN_FAILURE)
		{
			printf("%s\n", "move row faile");
			break;
		}
		else if(retval == RETURN_SUCCESS)
		{
			printf("-----------------------------\n");
			while(1)
			{
				retval = DBGetFieldValue(hstmt, value);
				if(retval == RETURN_FAILURE)
				{
					printf("Get value fail\n");
					return 0;
				}
				else if(retval == ORA_SQL_EXEC_RESULT_CODE_NOT_FOUND)
				{
					printf("该行没有更多数据了 %s\n", hstmt->error->errstr);
					break;
				}
				else
				{
					printf("value=%s\n", value);
				}
			}
		}
		else if(retval == ORA_SQL_EXEC_RESULT_CODE_NOT_FOUND)
		{
			printf("没有更多数据%s\n", hstmt->error->errstr);
			break;
		}
	}

	int j=0;
	for(i=0; i<row; i++)
	{
			printf("==============================\n");
		for(j=0; j<field; j++)
		{
			retval = DBGetFieldValueIdx( hstmt, i, j, value);
				if(retval == RETURN_FAILURE)
				{
					printf("Get value fail\n");
					return 0;
				}
				else if(retval == ORA_SQL_EXEC_RESULT_CODE_NOT_FOUND)
				{
					printf("该行没有更多数据了 %s\n", hstmt->error->errstr);
					break;
				}
				else
				{
					printf("value=%s\n", value);
				}
		}
	}

	DBDisconnect(hdbc);
	DBConnectFinalize(hdbc);

	DBUG_RETURN(0);
}
