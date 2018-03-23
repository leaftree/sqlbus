
/**
 * driver_loader.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-12 23:08:07
 * Last Modified : 2018-03-12 23:08:07
 */

#ifndef __DRIVER_LOADER_H__
#define __DRIVER_LOADER_H__

enum sqlbus_dm_type{
	DB_CONNECT_INITIALIZE = 1,
	DB_CONNECT_FINALIZE,
	DB_CONNECT,
	DB_DISCONNECT,
	DB_STMT_INITIALIZE,
	DB_STMT_FINALIZE,
	DB_STMT_EXECUTE,
	DB_STMT_GET_ROW_COUNT,
	DB_STMT_GET_FIELD_COUNT,
	DB_STMT_GET_FIELD_NAME,
	DB_STMT_GET_FIELD_LENGTH,
	DB_STMT_GET_NEXT_ROW,
	DB_STMT_GET_FIELD_VALUE,
	DB_STMT_GET_FIELD_VALUE_IDX,
	DB_STMT_GET_ERROR_MESSAGE,
	DB_DBC_GET_CONNECTION_STATUS,
};

typedef struct driver_func
{
	char *name;
	int (*func)();
}driver_func;

typedef struct driver_manager
{
	char *driver_name;
	void *dl_handle;
	driver_func *functions;
	char errstr[128];
} driver_manager, *HDM;

__BEGIN_DECLS

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
int load_driver(driver_manager *driver, char *file);

/**
 * unload_driver - 卸载驱动
 *
 * @driver_manager: 驱动信息
 *
 * return value
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 卸载驱动成功
 */
int unload_driver(driver_manager *driver);

__END_DECLS

#endif /* __DRIVER_LOADER_H__ */
