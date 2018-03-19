
/**
 * config_loader.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-14 10:41:51
 * Last Modified : 2018-03-14 10:41:51
 */

#ifndef __CONFIG_LOADER_H__
#define __CONFIG_LOADER_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/********************* 配置文件例子 ***********************
	[Oracle]
	Descriptor = Oracle Driver of Afc
	Driver     = ./oracle.so
	Server     =
	Port       =
	UserName   =
	Password   =
**********************************************************/

/**********************************************************
 *
 * 注意：提供获取配置信息的API返回的内容是Config链中的信息，
 *       因此只允许读取其值，不应该尝试去修改返回内容
 *
**********************************************************/

/**
 * separator
 */
#define DEFAULT_SEPARATOR '='

/**
 * annotator
 */
#define DEFAULT_ANNOTATOR ";#"

/**
 * conf_pair - 配置文中件的键值对
 * @name: 键名
 * @value: 值 
 * @ctext: 注释
 * @next: 下一个键值对节点
 */
typedef struct conf_pair
{
	char name[64];
	char value[128];
	char ctext[128];
	struct conf_pair *next;
} conf_pair_t;

/**
 * conf_section - 配置文件中的节
 * @name: 节名
 * @pair: 键值对链队头
 * @next: 下一个section节点
 */
typedef struct conf_section
{
	char name[64];
	conf_pair_t *tail;
	conf_pair_t *pair;
	struct conf_section *next;
} conf_section_t;

/**
 * configuration - 配置文件信息
 * @file: 文件名
 * @fp: 配置文件句柄
 * @section: section节点
 *
 * @fp 在加载完配置信息后会关闭，因此不应该操作该指针
 */
typedef struct configuration
{
	char file[512];
	FILE *fp;
	conf_section_t *section;
} config_t;

__BEGIN_DECLS

/**
 * load_config - 加载配置文件并解析文件
 * @file: 配置文件名，带路径
 * @conf: 配置信息
 *
 * return value:
 *  EXIT_SUCCESS - 加载成功
 *  EXIT_FAILURE - 加载出错
 */
int load_config(char *file, config_t *conf);

/**
 * unload_config - 清理已加载的配置项
 *
 * @conf: 配置项列表
 *
 * return value:
 *  RETURN_FAILURE: 出错
 *  RETURN_SUCCESS: 成功
 */
int unload_config(config_t *conf);

/**
 * get_config_section - 获取section节点
 *
 * @conf: 配置项列表
 * @name: 要获取的节点名称
 *
 * return value:
 *  NULL : 找不到section信息
 *  !(NULL) : section地址
 */
conf_section_t *get_config_section(const config_t *conf, const char *name);

/**
 * get_config_pair - 获取键值对
 *
 * @section: 指定要查找键值对的section
 * @name: 要查找的键值对名称
 *
 * return value:
 *  NULL : 找不到键值对信息
 *  !(NULL) : 返回匹配的键值对地址
 */
conf_pair_t *get_config_pair(const conf_section_t *section, char *name);

/**
 * get_config_value - 获取键的值
 *
 * @conf: 配置项列表
 * @section: section节点名称
 * @key: 键名称
 * @value: 键的值，输出值
 *
 * return value
 *  RETURN_FAILURE: 参数无效或者找不到对应的键值
 *  RETURN_SUCCESS: 发现匹配的键值，输出结果保存在@value中
 */
int get_config_value(const config_t *conf, char *section, char *key, char *value);

__END_DECLS

#endif /* __CONFIG_LOADER_H__ */
