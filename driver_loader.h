
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

#define DB_CONNECTINITIALIZE 1
#define DB_CONNECTFINISHED 2
#define DB_CONNECT 3
#define DB_DISCONNECT 4

typedef struct driver_func
{
	char *name;
	int (*func)();
}driver_func;

typedef struct driver_manager
{
	char *driver_name;
	void *dl_handle;
	driver_func *func;
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
