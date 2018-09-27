
/**
 * driver_loader.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-14 09:07:50
 * Last Modified : 2018-03-14 09:07:50
 */

#include <dlfcn.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "xmalloc.h"
#include "driver_loader.h"

/**
 * driver_func_template - 默认模板接口
 */
static driver_func driver_func_template[] = {
	[DB_CONNECT_INITIALIZE          ] = {"DBConnectInitialize"  , NULL},
	[DB_CONNECT                     ] = {"DBConnect"            , NULL},
	[DB_DISCONNECT                  ] = {"DBDisconnect"         , NULL},
	[DB_STMT_INITIALIZE             ] = {"DBStmtInitialize"     , NULL},
	[DB_STMT_FINALIZE               ] = {"DBStmtFinalize"       , NULL},
	[DB_STMT_EXECUTE                ] = {"DBExecute"            , NULL},
	[DB_STMT_GET_ROW_COUNT          ] = {"DBGetRowCount"        , NULL},
	[DB_STMT_GET_FIELD_COUNT        ] = {"DBGetFieldCount"      , NULL},
	[DB_STMT_GET_FIELD_NAME         ] = {"DBGetFieldNameIdx"    , NULL},
	[DB_STMT_GET_FIELD_LENGTH       ] = {"DBGetFieldLengthIdx"  , NULL},
	[DB_STMT_GET_NEXT_ROW           ] = {"DBGetNextRow"         , NULL},
	[DB_STMT_GET_FIELD_VALUE        ] = {"DBGetFieldValue"      , NULL},
	[DB_STMT_GET_FIELD_VALUE_IDX    ] = {"DBGetFieldValueIdx"   , NULL},
	[DB_STMT_GET_ERROR_MESSAGE      ] = {"DBGetErrorMessage"    , NULL},
	[DB_DBC_GET_CONNECTION_STATUS   ] = {"DBGetConnectionStatus", NULL},
	[DB_STMT_GET_EXECUTE_RESULT_CODE] = {"DBGetExecuteResultCode", NULL},
};

/**
 * open_driver - 打开驱动文件并读取接口
 *
 * @driver: 驱动信息
 *
 * return value
 *  RETURN_FAILURE: 参数无效或者读取指定接口失败
 *  RETURN_SUCCESS: 获取驱动接口成功
 */
static int open_driver(driver_manager *driver)
{
	if(driver == NULL || driver->driver_name == NULL)
		return(RETURN_FAILURE);

	driver->dl_handle = dlopen(driver->driver_name, RTLD_LAZY);
	if(driver->dl_handle == NULL)
	{
		sprintf(driver->errstr, "%s", dlerror());
		return(RETURN_FAILURE);
	}

	dlerror();
	driver->functions = xMalloc(sizeof(driver_func_template));
	if(driver->functions == NULL)
	{
		sprintf(driver->errstr, "%s", strerror(errno));
		return(RETURN_FAILURE);
	}
	memcpy(driver->functions, &driver_func_template, sizeof(driver_func_template));

	int i=0;
	for(i=0; i<sizeof(driver_func_template)/sizeof(driver_func_template[0]); i++)
	{
		if(driver->functions[i].name == NULL)
			continue;

		driver->functions[i].func = dlsym(driver->dl_handle, driver->functions[i].name);
		if (driver->functions[i].func == NULL)
		{
			sprintf(driver->errstr, "%s", dlerror());
			return(RETURN_FAILURE);
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * load_driver - 加载数据库驱动
 *
 * @driver: 驱动信息
 * @file: 驱动文件，包含路径
 *
 * return value
 *  RETURN_FAILURE: 参数无效或者加载驱动失败
 *  RETURN_SUCCESS: 加载驱动成功
 */
int load_driver(driver_manager *driver, char *file)
{
	if(driver == NULL || file == NULL)
		return(RETURN_FAILURE);

	driver->driver_name = xStrdup(file);

	return(open_driver(driver));

	//return(RETURN_SUCCESS);
}

/**
 * unload_driver - 卸载驱动
 *
 * @driver_manager: 驱动信息
 *
 * return value
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 卸载驱动成功
 */
int unload_driver(driver_manager *driver)
{
	if(driver == NULL)
		return(RETURN_FAILURE);

	xFree(driver->driver_name);
	xFree(driver->functions);
	if(driver->dl_handle)
		dlclose(driver->dl_handle);
	driver->errstr[0] = 0;

	return(RETURN_SUCCESS);
}
