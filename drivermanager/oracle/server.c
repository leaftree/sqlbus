
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

	DBDisconnect(hdbc);
	DBConnectFinished(hdbc);

	DBUG_RETURN(0);
}
