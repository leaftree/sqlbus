
/**
 * util.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-10 15:19:55
 * Last Modified : 2018-04-10 15:19:55
 */

#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/prctl.h>

#include "../third/cJSON/cJSON.h"
#include "hisqlbus.h"

/**
 * getUuid - 获取v4版本UUID，从内核中读取
 */
int getUuid(char **uuid)
{
#define UUID_SOURCE "/proc/sys/kernel/random/uuid"
#define UUID_LENGTH (sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")-1)

	int size = 0;
	char uuid4[UUID_LENGTH+1] = "";

	int fd = open(UUID_SOURCE, O_RDONLY);
	if(fd<0) {
		return(FAILURE);
	}

	size = read(fd, uuid4, UUID_LENGTH);
	if(size != UUID_LENGTH) {
		close(fd);
		return(FAILURE);
	}

	*uuid = strdup(uuid4);
	close(fd);

	return(SUCCESS);
}

/**
 * getTimestamp - 获取当前时间，单位为秒
 */
int getTimestamp(unsigned int *ct)
{
	return *ct = time(NULL);
}

/**
 * getAppName - 获取进程名称
 */
int getAppName(char **name)
{
	char app[64] = "";
	prctl(PR_GET_NAME, app);
	*name = strdup(app);
	return(SUCCESS);
}

/**
 * getPid - 获取进程Id
 */
int getPid(unsigned int *pid)
{
	return *pid = getpid();
}

/**
 * checkReturnValueArrayIsValid - 检查函数返回值数组中是否包含失败的返回值
 * 注意：带信息的返回值不算是失败的返回值
 */
static int checkReturnValueArrayIsValid(int array[], int cnt, int *idx)
{
	int i;
	for(i=0; i<cnt; i++) {
		if(!SUCCESSED(array[i])) {
			if(idx) {
				*idx = i;
			}
			return(FAILURE);
		}
	}
	return(SUCCESS);
}

/**
 * addStringToCJson - 向Json结构中添加一个字符串类型的节点
 */
static int addStringToCJson(cJSON *root, char *key, char *value)
{
	if(root == NULL || key == NULL || value == NULL) {
		return(FAILURE);
	}

	cJSON *leaf = cJSON_CreateString(value);
	if(leaf == NULL) {
		return(FAILURE);
	}
	cJSON_AddItemToObject(root, key, leaf);

	return(SUCCESS);
}

/**
 * addNumberToCJson - 向Json结构中添加一个Int类型的节点
 */
static int addNumberToCJson(cJSON *root, char *key, uint32_t value)
{
	if(root == NULL || key == NULL) {
		return(FAILURE);
	}

	cJSON *leaf = cJSON_CreateNumber(value);
	if(leaf == NULL) {
		return(FAILURE);
	}
	cJSON_AddItemToObject(root, key, leaf);

	return(SUCCESS);
}

/**
 * addBooleanToCJson - 向Json结构中添加一个boolean类型的节点
 */
static int addBooleanToCJson(cJSON *root, char *key, uint32_t value)
{
	if(root == NULL || key == NULL) {
		return(FAILURE);
	}

	cJSON *leaf = NULL;

	if(value) {
		leaf = cJSON_CreateTrue();
	}
	else {
		leaf = cJSON_CreateFalse();
	}
	if(leaf == NULL) {
		return(FAILURE);
	}
	cJSON_AddItemToObject(root, key, leaf);

	return(SUCCESS);
}

/**
 * marshallJson - 序列化一个Json结构
 */
int marshallJson(sqlbus *bus, char **requestString)
{
	if(!bus || !bus->proto || !bus->proto || !requestString) {
		return(FAILURE);
	}

	int retIdx = 0;
	int retVal[1024] = { 0 };
	sqlbusRequest *request = &bus->proto->request;
	cJSON *leaf, *root = cJSON_CreateObject();

	retVal[retIdx++] = addStringToCJson(root, "app", request->clientName);
	retVal[retIdx++] = addStringToCJson(root, "uuid", request->universalUniqueIdentification);
	retVal[retIdx++] = addNumberToCJson(root, "pid", request->processIdentification);
	retVal[retIdx++] = addStringToCJson(root, "type", request->messageType);
	retVal[retIdx++] = addBooleanToCJson(root, "sync", !request->synchronizationType);
	retVal[retIdx++] = addStringToCJson(root, "rchannel", request->receiveChannel);
	retVal[retIdx++] = addNumberToCJson(root, "timestamp", request->timestamp);

	if(request->requestStatement.statement)
	{
		printf("------\n");
		retVal[retIdx++] = addStringToCJson(root, "statement", request->requestStatement.statement);
	}
	else
	{
		printf("======\n");
		leaf = cJSON_CreateObject();
		retVal[retIdx++] = addStringToCJson(leaf, "update", request->requestStatement.ui.update);
		retVal[retIdx++] = addStringToCJson(leaf, "insert", request->requestStatement.ui.insert);
		cJSON_AddItemToObject(root, "statement", leaf);
	}

	if(checkReturnValueArrayIsValid(retVal, retIdx, NULL))
	{
		cJSON_free(root);
		return(FAILURE);
	}
	else
	{
		*requestString = cJSON_PrintUnformatted(root);
		cJSON_free(root);
	}

	return(SUCCESS);
}

/**
 * getCJsonValueAsString - 读取Json结构中一个字符串类型的节点，由@key指点节点
 * 并且只读取第一个@key节点，其节点的值返回给@value
 */
static int getCJsonValueAsString(cJSON *root, int ignore, char *key, char **value)
{
	cJSON *leaf = NULL;

	if(root == NULL || key == NULL || value == NULL) {
		return(FAILURE);
	}

	leaf = cJSON_GetObjectItem(root, key);
	if(leaf == NULL || cJSON_IsString(leaf) == false) {
		if(ignore) {
			return(SUCCESS_WITH_INFO);
		}
		return(FAILURE);
	}

	*value = strdup(leaf->valuestring);
	return(SUCCESS);
}

/**
 * getCJsonValueAsNumber - 读取Json结构中一个Number类型的节点，由@key指点节点
 * 并且只读取第一个@key节点，其节点的值返回给@value
 */
static int getCJsonValueAsNumber(cJSON *root, int ignore, char *key, unsigned int *value)
{
	cJSON *leaf = NULL;

	if(root == NULL || key == NULL || value == NULL) {
		return(FAILURE);
	}

	leaf = cJSON_GetObjectItem(root, key);
	if(leaf == NULL || cJSON_IsNumber(leaf) == false) {
		if(ignore) {
			return(SUCCESS_WITH_INFO);
		}
		return(FAILURE);
	}

	*value = leaf->valueint;
	return(SUCCESS);
}

/**
 * getCJsonValueAsArray - 读取Json结构中一个Array类型的节点，由@key指点节点
 * 并且只读取第一个@key节点，@leaf用来存储该Array节点
 */
static int getCJsonValueAsArray(cJSON *root, char *key, cJSON **leaf)
{
	if(root == NULL || key == NULL || leaf == NULL) {
		return(FAILURE);
	}

	cJSON *lf = cJSON_GetObjectItem(root, key);
	if(lf == NULL || cJSON_IsArray(lf) == false) {
		if(lf == NULL)
			printf("err %d %s\n", __LINE__, cJSON_GetErrorPtr());
		else
			printf("err %d item->type=%d\n", __LINE__, lf->type);
		return(FAILURE);
	}

	*leaf = lf;

	return(SUCCESS);
}

/**
 * checkCJsonArrayMemberNumber - 检查Json的Array类型中成员的个数是否与@cnt一致
 */
static int checkCJsonArrayMemberNumber(cJSON *root, int cnt)
{
	if(root == NULL) {
		return(FAILURE);
	}

	if(cJSON_GetArraySize(root) != cnt) {
		return(FAILURE);
	}
	return(SUCCESS);
}

// 注意：不能释放Json串的根节点
// 为了节省空间和提高反序列化速度，取得字段和行值后用cJson节点保存
// 读取字段信息及行信息时从cJson节点中读取
// 在完成该次数据库操作后释放资源时一起释放
int unMarshallJson(sqlbus *bus, char *responseString)
{
	if(!bus || !bus->proto || !responseString) {
		return(FAILURE);
	}

	int retIdx = 0;
	int errIdx = -1;
	int retVal[20] = { 0 };
	cJSON *root = NULL, *fleaf = NULL, *rleaf = NULL;
	sqlbusResponse *response = &bus->proto->response;

	root = cJSON_Parse(responseString);
	if(root == NULL) {
		bus->error.etag = 1;
		sprintf(bus->error.errmsg, "%s", cJSON_GetErrorPtr());
		return(FAILURE);
	}

	retVal[retIdx++] = getCJsonValueAsString(root, true, "type", &response->messageType);
	retVal[retIdx++] = getCJsonValueAsString(root, true, "app", &response->serverName);
	retVal[retIdx++] = getCJsonValueAsString(root, true, "dbtype", &response->databaseType);
	retVal[retIdx++] = getCJsonValueAsString(root, true, "uuid", &response->universalUniqueIdentification);
	retVal[retIdx++] = getCJsonValueAsString(root, true, "message", &response->messageInfo);
	retVal[retIdx++] = getCJsonValueAsNumber(root, true, "pid", &response->processIdentification);
	retVal[retIdx++] = getCJsonValueAsNumber(root, true, "timestamp", &response->timestamp);
	retVal[retIdx++] = getCJsonValueAsNumber(root, true, "errorid", &response->messageIdentification);
	retVal[retIdx++] = getCJsonValueAsNumber(root, false, "fields", &response->fieldCounter);
	retVal[retIdx++] = getCJsonValueAsNumber(root, false, "rows", &response->rowCounter);
	retVal[retIdx++] = getCJsonValueAsArray(root, "field", &fleaf);
	retVal[retIdx++] = getCJsonValueAsArray(root, "result", &rleaf);
	retVal[retIdx++] = checkCJsonArrayMemberNumber(fleaf, response->fieldCounter);
	retVal[retIdx++] = checkCJsonArrayMemberNumber(rleaf, response->rowCounter);

	if(checkReturnValueArrayIsValid(retVal, retIdx, &errIdx))
	{
		cJSON_free(root);
		bus->error.etag = 1;
		sprintf(bus->error.errmsg, "Parse json string failed for index(%d)", errIdx);
		return(FAILURE);
	}

	response->rowData = rleaf;
	response->fieldData = fleaf;
	response->reserveData = root;

	return(SUCCESS);
}

int releaseJsonSource(void *data)
{
	if(!data) {
		return(FAILURE);
	}

	cJSON *root = (cJSON*)data;
	cJSON_free(root);

	return(SUCCESS);
}

int getFieldName(void *data, int idx, char **name)
{
	if(!data || !name) {
		return(FAILURE);
	}

	cJSON *leaf = NULL;
	cJSON *root = (cJSON*)data;

	if(cJSON_IsArray(root) != true) {
		return(FAILURE);
	}

	leaf = cJSON_GetArrayItem(root, idx);
	if(leaf == NULL || cJSON_IsString(leaf) != true) {
		return(FAILURE);
	}

	*name = leaf->valuestring;

	return(SUCCESS);
}

int getFieldIdx(void *data, char *fname, int *idx)
{
	int i=0;
	int cnt=0;

	if(!data || !fname || !idx) {
		return(FAILURE);
	}

	cJSON *leaf = NULL;
	cJSON *root = (cJSON*)data;

	if(cJSON_IsArray(root) != true) {
		return(FAILURE);
	}

	cnt = cJSON_GetArraySize(root);

	for(i=0; i<cnt; i++)
	{
		leaf = cJSON_GetArrayItem(root, i);
		if(leaf == NULL || cJSON_IsString(leaf) != true) {
			return(FAILURE);
		}

		if(!strcasecmp(fname, leaf->valuestring)) {
			*idx = i;
			return(SUCCESS);
		}
	}

	return(FAILURE);
}

int getRowValue(void *data, int ridx, int cidx, char **value)
{
	if(!data || !value) {
		return(FAILURE);
	}

	cJSON *leaf = NULL;
	cJSON *root = (cJSON*)data;

	if(cJSON_IsArray(root) != true) {
		return(FAILURE);
	}

	leaf = cJSON_GetArrayItem(root, ridx);
	if(leaf == NULL || cJSON_IsArray(leaf) != true) {
		return(FAILURE);
	}

	leaf = cJSON_GetArrayItem(leaf, cidx);
	if(leaf == NULL || cJSON_IsString(leaf) != true) {
		return(FAILURE);
	}
	*value = leaf->valuestring;

	return(SUCCESS);
}
