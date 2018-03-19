
/**
 * config_loader.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-14 11:09:18
 * Last Modified : 2018-03-14 11:09:18
 */

#include "util.h"
#include "dbug.h"
#include "config_loader.h"

/**
 * pair_new - 申请一个新的键值对节点
 *
 * @name: 节点名称
 *
 * return value:
 *  新pair节点地址空间
 */
static conf_pair_t *pair_new(char *name, char *value, char *ctext)
{
	conf_pair_t *pair = malloc(sizeof(conf_pair_t));
	memset(pair, 0x0, sizeof(conf_pair_t));
	sprintf(pair->name, "%s", name?name:"");
	sprintf(pair->value, "%s", value?value:"");
	sprintf(pair->ctext, "%s", ctext?ctext:"");
	pair->next = NULL;

	return pair;
}

/**
 * section_new - 申请一个新的section节点
 *
 * @name: 节点名称
 *
 * return value:
 *  新section节点地址空间
 */
static conf_section_t *section_new(char *name)
{
	conf_section_t *section = malloc(sizeof(conf_section_t));
	memset(section, 0x0, sizeof(conf_section_t));
	sprintf(section->name, "%s", name?name:"");
	section->next = NULL;
	section->pair = NULL;

	return section;
}

/**
 * append_pair_to_section - 向节点中添加键值对
 *
 * @section: 节点
 * @pair: 键值对
 *
 * return value:
 *  RETURN_SUCCESS: 键值对成功插入到节中
 *  RETURN_FAILURE: 键值对插入到节中失败
 */
static int append_pair_to_section(conf_section_t *section, conf_pair_t *pair)
{
	if(section == NULL || pair == NULL)
		return(RETURN_FAILURE);

	pair->next = NULL;

	if(section->pair == NULL)
	{
		section->pair = pair;
		section->tail = pair;
	}
	else
	{
		section->tail->next = pair;
		section->tail = pair;
	}
	return(RETURN_SUCCESS);
}

/**
 * append_section_to_config - 将section节点添加到配置列表中(内存)
 *
 * @conf: 配置列表
 * @section: section节点
 *
 * return value
 */
static int append_section_to_config(config_t *conf, conf_section_t *section)
{
	conf_section_t *sct = NULL;

	if(conf == NULL || section == NULL)
		return(RETURN_FAILURE);

	section->next = NULL;

	if(conf->section == NULL)
	{
		conf->section = section;
	}
	else
	{
		sct = conf->section;
		while(sct->next)
			sct = sct->next;
		sct->next = section;
	}

	return(RETURN_SUCCESS);
}

/**
 * open_conf_file - 打开配置文件
 *
 * @file: 配置文件名称，带路径
 *
 * 如果@file为空，测尝试打开默认配置/etc/sqlbus.ini
 *
 * return value:
 *  NULL: 打开文件失败
 *  !(NULL): @file的文件指针
 */
static FILE *open_conf_file(char *file)
{
	FILE *fp = NULL;

	if(file)
		fp = fopen(file, "r+");
	else
		fp = fopen("/etc/sqlbus.ini", "r+");

	return fp;
}

/**
 * close_conf_file - 关闭配置文件句柄
 *
 * @fp: 文件句柄
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者关闭失败
 *  RETURN_SUCCESS：文件句柄关闭成功
 */
static int close_conf_file(FILE *fp)
{
	if(fp == NULL)
		return(RETURN_FAILURE);

	if(fclose(fp) != 0)
		return(RETURN_FAILURE);

	return(RETURN_SUCCESS);
}

/**
 * read_one_line - 从文件指针@fp中读取一行内容
 *
 * @fp: 已打开的文件指针
 * @line: 行缓冲空间
 * @size: @line空间的长度
 *
 * return value:
 *  RETURN_FAILURE: @fp或者@line无效
 *  RETURN_SUCCESS: 文件内容读取完
 *  >0 : 读取到一行内容，返回其长度
 */
static int read_one_line(FILE *fp, char *line, int size)
{
	if(fp == NULL || line == NULL)
		return(RETURN_FAILURE);

	char *ptr = fgets(line, size, fp);
	if(ptr == NULL)
	{
		if(feof(fp))
			return(0);
	}
	return strlen(line);
}

/**
 * trim_space_character - 删除@string左边和右边的空白符及换行回车符
 *
 * @string: 被删除空白符和换行回车符的字符串
 *
 * @trim_space_character会修改输入参数内容
 *
 * return value
 *  RETURN_FAILURE: @string无效
 *  >= 0 : 删除符号后的字符串长度
 */
static int trim_space_character(char *string)
{
	if(string == NULL)
		return(RETURN_FAILURE);

	rtrim(string, strlen(string));
	ltrim(string, strlen(string));

	return strlen(string);
}

/**
 * is_blank_line - 判断@string是否为空字符串
 *
 * @string: 
 *
 * return value:
 *  RETURN_SUCCESS: @string是空字符串
 *  RETURN_FAILURE: @string不是空字符串
 */
static int is_blank_line(char *string)
{
	if(string == NULL || *string == 0x0)
		return(RETURN_SUCCESS);
	return(RETURN_FAILURE);
}

/**
 * is_comment_line - 判断@string是否为注释行
 * 
 * @string:
 *
 * 当@string是空字符串时，当作注释行看待
 *
 * return value:
 *  RETURN_SUCCESS: @string是注释行
 *  RETURN_FAILURE: @string不是注释行
 */
static int is_comment_line(char *string)
{
	int i=0;

	if(string == NULL)
		return(RETURN_SUCCESS);

	for(i=0; i<strlen(DEFAULT_ANNOTATOR); i++)
	{
		if(*string == DEFAULT_ANNOTATOR[i])
			return(RETURN_SUCCESS);
	}
	return(RETURN_FAILURE);
}

/**
 * is_section - 判断@string是否为"节"
 *
 * @string: 文件行内容
 * @name: "节"名称
 *
 * @string为空字符串时，非"节"行
 * @name仅当@string为节行时有效，是出参，用于存储"节"名称
 *
 * return value:
 *  RETURN_FAILURE: 不是"节"行
 *  RETURN_SUCCESS：是"节"行
 */
static int is_section(char *string, char *name)
{
	if(string == NULL)
		return RETURN_FAILURE;

	if(*string == '[')
	{
		char *start = string;
		char *end = start + strlen(start);

		while(++start != end && *start != ']') {
			;
		}
		snprintf(name, start-string, "%s", string+1);

		return RETURN_SUCCESS;
	}
	return RETURN_FAILURE;
}

/**
 * separate_key_value_pair - 将字符串@string分割为键值对
 *
 * @string: 待分割字符串
 * @pair: 键值对，为2级指针
 *
 * 当@string可以分割出pair时，@separate_key_value_pair会为@pair分配空间
 * 因此，外层调用应该释放其空间
 *
 * return value:
 *  RETURN_FAILURE: 参数无效/不能分割出键值对
 *  RETURN_SUCCESS: 分割成功
 */
static int separate_key_value_pair(char *string, conf_pair_t **pair)
{
	DBUG_ENTER(__func__);

	char key[512] = "";
	char value[512] = "";

	if(string == NULL )
		DBUG_RETURN(RETURN_FAILURE);

	char *end = string + strlen(string);
	char *ptr = index(string, DEFAULT_SEPARATOR);
	if(ptr == NULL) {
		DBUG_RETURN(RETURN_FAILURE);
	}

	snprintf(key, abs(ptr-string)+1, "%s", string);
	rtrim(key, strlen(key));

	if(ptr+1 == NULL) {
		*value = 0x0;
	}
	else
	{
		snprintf(value, abs(end-ptr), "%s", ptr+1);
		ltrim(value, strlen(value));
	}

	*pair = pair_new(key, value, NULL);

	DBUG_RETURN(RETURN_SUCCESS);
}

/**
 * parase_conf_file - 逐行读取配置文件内容，并解析
 *
 * @conf: 配置文件内容
 *
 * return value:
 *  RETURN_FAILURE: 入参无效或者读取文件内容失败
 *  RETURN_SUCCESS: 解析配置文件成功
 */
static int parase_conf_file(config_t *conf)
{
	int retval = 0;
	int has_section = 0;
	char line[512] = "";
	char section_name[256] = "";

	conf_pair_t *pair;
	conf_section_t *section;

	if(conf == NULL || conf->fp == NULL)
		return(RETURN_FAILURE);

	while(1)
	{
		retval = read_one_line(conf->fp, line, sizeof(line));
		if(retval < 0) {
			return(RETURN_FAILURE);
		}
		if(retval == 0) {
			break;
		}

		trim_space_character(line);

		if(is_blank_line(line) == RETURN_SUCCESS) {
			continue;
		}
		if(is_comment_line(line) == RETURN_SUCCESS) {
			continue;
		}
		if(is_section(line, section_name) == RETURN_SUCCESS) {
			has_section ++;
			section = section_new(section_name);
			append_section_to_config(conf, section);
			continue;
		}

		if(!has_section) // 每对键值对都应该有属于自己的section
			continue;

		if(separate_key_value_pair(line, &pair) == RETURN_FAILURE) {
			continue;
		}

		append_pair_to_section(section, pair);
	}
}

/**
 * load_config - 加载配置文件
 *
 * @file: 配置文件名称，带路径
 * @conf: 配置文件表项
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者打开文件失败
 *  RETURN_SUCCESS: 加载配置文件成功
 */
int load_config(char *file, config_t *conf)
{
	DBUG_ENTER(__func__);

	int retval = 0;

	conf->fp = open_conf_file(file);
	if(conf->fp == NULL)
	{
		return(RETURN_FAILURE);
	}

	retval = parase_conf_file(conf);

	close_conf_file(conf->fp);

	DBUG_RETURN(retval);
}

/**
 * unload_config - 清理已加载的配置项
 *
 * @conf: 配置项列表
 *
 * return value:
 *  RETURN_FAILURE: 出错
 *  RETURN_SUCCESS: 成功
 */
int unload_config(config_t *conf)
{
	DBUG_ENTER(__func__);

	conf_pair_t *pair = NULL, *tpair = NULL;
	conf_section_t *section = NULL, *tsection = NULL;

	if(conf == NULL || conf->section == NULL)
	{
		DBUG_RETURN(RETURN_FAILURE);
	}

	section = conf->section;
	while(section)
	{
		pair = section->pair;
		while(pair)
		{
			tpair = pair;
			pair = pair->next;
			free(tpair);
		}
		tsection = section;
		section = section->next;
		free(tsection);
	}

	DBUG_RETURN(RETURN_SUCCESS);
}

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
conf_section_t *get_config_section(const config_t *conf, const char *name)
{
	if(conf == NULL || conf->section == NULL || name == NULL)
		return(NULL);

	conf_section_t *section = conf->section;

	while(section)
	{
		if(strcasecmp(section->name, name) == 0) {
			return section;
		}
		else {
			section = section->next;
		}
	}
	return(NULL);
}

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
conf_pair_t *get_config_pair(const conf_section_t *section, char *name)
{
	if(section == NULL || name == NULL)
		return(NULL);

	conf_pair_t *pair = section->pair;

	while(pair)
	{
		if(strcasecmp(pair->name, name) == 0) {
			return(pair);
		}
		pair = pair->next;
	}

	return(NULL);
}

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
int get_config_value(const config_t *conf, char *section, char *key, char *value)
{
	DBUG_ENTER(__func__);

	if(conf == NULL || conf->section == NULL ||
			section == NULL || key == NULL || value == NULL)
		DBUG_RETURN(RETURN_FAILURE);

	conf_section_t *sct = get_config_section(conf, section);
	if(sct == NULL) {
		DBUG_PRINT("section", ("no section"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	conf_pair_t *pair = get_config_pair(sct, key);
	if(pair == NULL) {
		DBUG_PRINT("pair", ("no pair"));
		DBUG_RETURN(RETURN_FAILURE);
	}

	sprintf(value, "%s", pair->value);
	DBUG_RETURN(RETURN_SUCCESS);
}
