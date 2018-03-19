
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

#include "dbug.h"
#include "util.h"
#include "driver_loader.h"

/**
 * driver_func_template - 默认模板接口
 */
static driver_func driver_func_template[] = {
	[DB_CONNECTINITIALIZE] = {"DBConnectInitialize", NULL},
	[DB_CONNECTFINISHED  ] = {"DBConnectFinished"  , NULL},
	//[DB_CONNECT          ] = {"DBConnect"          , NULL},
	[DB_DISCONNECT       ] = {"DBDisconnect"       , NULL},
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
	DBUG_ENTER(__func__);

	driver->dl_handle = dlopen(driver->driver_name, RTLD_LAZY);
	if(driver->dl_handle == NULL)
	{
		sprintf(driver->errstr, "%s", dlerror());
		DBUG_RETURN(RETURN_FAILURE);
	}

	dlerror();
	driver->func = malloc(sizeof(driver_func_template));
	if(driver->func == NULL)
	{
		sprintf(driver->errstr, "%s", strerror(errno));
		DBUG_RETURN(RETURN_FAILURE);
	}
	memcpy(driver->func, &driver_func_template, sizeof(driver_func_template));

	int i=0;
	for(i=0; i<sizeof(driver_func_template)/sizeof(driver_func_template[0]); i++)
	{
		if(driver->func[i].name == NULL)
			continue;

		driver->func[i].func = dlsym(driver->dl_handle, driver->func[i].name);
		if (driver->func[i].func == NULL)
		{
			sprintf(driver->errstr, "%s", dlerror());
			DBUG_RETURN(RETURN_FAILURE);
		}
	}

	DBUG_RETURN(RETURN_SUCCESS);
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
	DBUG_ENTER(__func__);

	if(driver == NULL || file == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	driver->driver_name = strdup(file);

	DBUG_RETURN(open_driver(driver));

	//DBUG_RETURN(RETURN_SUCCESS);
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
	DBUG_ENTER(__func__);

	if(driver == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	mFree(driver->driver_name);
	mFree(driver->func);
	dlclose(driver->dl_handle);
	driver->errstr[0] = 0;

	DBUG_RETURN(RETURN_SUCCESS);
}
