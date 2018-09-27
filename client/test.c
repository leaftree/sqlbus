
/**
 * test.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-11 16:41:58
 * Last Modified : 2018-04-11 16:41:58
 */

#include <stdlib.h>
#include "util.h"
#include "hisqlbus.h"

// Debug log func

char hostname[64] = "10.15.121.66";
char password[64] = "cssweb123";

#ifdef HI_SQL_BUS_DEBUG_MAIN
void logInfo(const char *fmt, ...)
{
	char log[16*1024] = "";
	va_list ap;
	va_start(ap, fmt);
	vsprintf(log, fmt, ap);
	va_end(ap);

	fprintf(stdout, "[ \e[1;32mINFO\e[0m ] %s\n", log);
}

void logError(const char *fmt, ...)
{
	char log[16*1024] = "";
	va_list ap;
	va_start(ap, fmt);
	vsprintf(log, fmt, ap);
	va_end(ap);

	fprintf(stderr, "[ \e[1;31mEROR\e[0m ] %s\n", log);
}

void logDebug(const char *fmt, ...)
{
	char log[16*1024] = "";
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(log, 16*1024, fmt, ap);
	va_end(ap);

	fprintf(stdout, "[ \e[1;33mDBUG\e[0m ] %s\n", log);
}

struct Log {
	void (*Info)(const char *fmt, ...);
	void (*Debug)(const char *fmt, ...);
	void (*Error)(const char *fmt, ...);
} Log = {
	.Info = logInfo,
	.Error = logError,
	.Debug = logDebug,
};

void OperError(sqlbus *bus, char *fmt, ...) {
	char msg[1024] = "";
	va_list ap;
	va_start(ap, fmt);
	vsprintf(msg, fmt, ap);
	va_end(ap);
	Log.Error("Operation failure:%s:%s", msg, bus->error.errmsg);
}

void get_password()
{
	char *ptr = getenv("RPASSWORD");
	if(ptr) {
		sprintf(password, "%s", ptr);
	}
	else {
		printf("%s:", "Input password of redis");
		if(scanf("%s", password)<0)
			return;
	}
}
void get_hostname()
{
	char *ptr = getenv("RHOSTNAME");
	if(ptr) {
		sprintf(hostname, "%s", ptr);
	}
	else {
		printf("%s:", "Input hostname of redis");
		if(scanf("%s", hostname)<0)
			return;
	}
}

int connect_2_memcache_server()
{
	int bOk = 0;

	// 创建句柄
	sqlbus *bus = sqlbusCreate(0, hostname, password);
	if(!bus) {
		OperError(bus, "%s", "Create sqlbus client failure.");
		return(FAILURE);
	}
	Log.Info("Init sqlbus client succ.");

	// 连接缓存服务
	bOk = bus->ofn->conn(bus);
	if(bOk) {
		OperError(bus, "%s", "Connect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Connect OK.");

	// 关闭连接
	bOk = bus->ofn->close(bus);
	if(bOk) {
		OperError(bus, "%s", "Disconnect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Close OK.");

	sqlbusFinalize(bus);

	return(SUCCESS);
}

int connect_to_memcache_server_and_auth_permission()
{
	int bOk = 0;

	// 创建句柄
	sqlbus *bus = sqlbusCreate(0, hostname, password);
	if(!bus) {
		OperError(bus, "%s", "Create sqlbus client failure.");
		return(FAILURE);
	}
	Log.Info("Init sqlbus client succ.");

	// 连接缓存服务
	bOk = bus->ofn->conn(bus);
	if(bOk) {
		OperError(bus, "%s", "Connect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Connect OK.");

	// 服务认证
	bOk = bus->ofn->auth(bus);
	if(bOk) {
		OperError(bus, "%s", "Auth memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Auth OK.");

	// 关闭连接
	bOk = bus->ofn->close(bus);
	if(bOk) {
		OperError(bus, "%s", "Disconnect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Close OK.");

	sqlbusFinalize(bus);

	return(SUCCESS);
}

int connect_to_memcache_server_and_select_instance()
{
	int bOk = 0;

	// 创建句柄
	sqlbus *bus = sqlbusCreate(0, hostname, password);
	if(!bus) {
		OperError(bus, "%s", "Create sqlbus client failure.");
		return(FAILURE);
	}
	Log.Info("Init sqlbus client succ.");

	// 连接缓存服务
	bOk = bus->ofn->conn(bus);
	if(bOk) {
		OperError(bus, "%s", "Connect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Connect OK.");

	// 服务认证
	bOk = bus->ofn->auth(bus);
	if(bOk) {
		OperError(bus, "%s", "Auth memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Auth OK.");

	// 选择实例
	bOk = bus->ofn->select(bus, "0");
	if(bOk) {
		OperError(bus, "%s", "Use memcache server instance failure.");
		return(FAILURE);
	}
	Log.Info("Select OK.");

	// 关闭连接
	bOk = bus->ofn->close(bus);
	if(bOk) {
		OperError(bus, "%s", "Disconnect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Close OK.");

	sqlbusFinalize(bus);

	return(SUCCESS);
}

int execute_an_query_sql_statement()
{
	int i, j;
	int bOk = 0;
	int rows = 0, fields = 0;
	char *row = NULL, *field = NULL;

	// 创建句柄
	sqlbus *bus = sqlbusCreate(0, hostname, password);
	if(!bus) {
		OperError(bus, "%s", "Create sqlbus client failure.");
		return(FAILURE);
	}
	Log.Info("Init sqlbus client succ.");

	// 连接缓存服务
	bOk = bus->ofn->conn(bus);
	if(bOk) {
		OperError(bus, "%s", "Connect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Connect OK.");

	// 服务认证
	bOk = bus->ofn->auth(bus);
	if(bOk) {
		OperError(bus, "%s", "Auth memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Auth OK.");

	// 发送请求，设置为同步请求，请求sqlbus响应请求
	bOk = bus->ofn->send(bus, "select * from basi_station_info", EXEC_SYNC);
	//bOk = bus->ofn->send(bus, "select * from pass_flow_info_his", EXEC_SYNC);
	if(bOk) {
		OperError(bus, "%s", "Request send to memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Send OK.");

	// 读取sqlbus反馈
	bOk = bus->ofn->recv(bus);
	if(bOk) {
		OperError(bus, "%s", "Waiting to recv to memcache server response failure.");
		return(FAILURE);
	}
	Log.Info("Recv OK.");

	bOk = bus->rfn->perfect(bus);
	if(!SUCCESSED(bOk)) {
		OperError(bus, "%s", "perfect run failure.");
		return(FAILURE);
	}
	else if(bOk == 1)
	{
		Log.Info("SQL execute failure.%s", bus->error.errmsg);
		return(SUCCESS);
	}
	Log.Info("Fetch data OK.");
	Log.Debug("响应数据:%s.", bus->proto->response.originData);

	// 获取SQL选择列数
	bOk = bus->rfn->fcount(bus, &fields);
	if(bOk) {
		OperError(bus, "%s", "Get column number failure.");
		return(FAILURE);
	}
	Log.Info("有%d个字段", fields);

	// show all field name
	for(i=0; i<fields; i++) {
		bOk = bus->rfn->fname(bus, i, &field);
		if(bOk) {
			OperError(bus, "%s", "Get row number failure.");
			return(FAILURE);
		}
		Log.Info("第%d字段是%s", i, field);
	}

	// 获取SQL查询结果行数
	bOk = bus->rfn->rcount(bus, &rows);
	if(bOk) {
		OperError(bus, "%s", "Get row number failure.");
		return(FAILURE);
	}
	Log.Info("有%d行记录", rows);

	for(i=0; i<rows; i++)
	{
		for(j=0; j<fields; j++)
		{
			bOk = bus->rfn->fname(bus, j, &field);
			if(bOk) {
				OperError(bus, "Get fieldname by index failure.");
				return(FAILURE);
			}

			bOk = bus->rfn->value(bus, i, field, &row);
			if(bOk) {
				OperError(bus, "Get field value for row(%d) by field name[%s] failure.", i, field);
				return(FAILURE);
			}
			Log.Info("第%d行第%d字段值为%s", i, j, row);
		}
	}

	// 关闭连接
	bOk = bus->ofn->close(bus);
	if(bOk) {
		OperError(bus, "%s", "Disconnect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Close OK.");

	sqlbusFinalize(bus);

	return(SUCCESS);
}

int execute_mutil_sql_statement()
{
	int bOk = 0;

	// 创建句柄
	sqlbus *bus = sqlbusCreate(0, hostname, password);
	if(!bus) {
		OperError(bus, "%s", "Create sqlbus client failure.");
		return(FAILURE);
	}
	Log.Info("Init sqlbus client succ.");

	// 连接缓存服务
	bOk = bus->ofn->conn(bus);
	if(bOk) {
		OperError(bus, "%s", "Connect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Connect OK.");

	// 服务认证
	bOk = bus->ofn->auth(bus);
	if(bOk) {
		OperError(bus, "%s", "Auth memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Auth OK.");

	// 发送包含多条语句的请求
	bOk = bus->ofn->uisend(bus,
			"update mode_status_info set mode_code = '0800' where station_id = '0100'",
			"insert into mode_status_info values('01', '0100', '0800', '1', '20180602', '120001')");
	if(bOk) {
		OperError(bus, "%s", "Request send to memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Send OK.");

	// 关闭连接
	bOk = bus->ofn->close(bus);
	if(bOk) {
		OperError(bus, "%s", "Disconnect memcache server failure.");
		return(FAILURE);
	}
	Log.Info("Close OK.");

	sqlbusFinalize(bus);

	return(SUCCESS);
}

struct TestCase {
	int retval;
	char *name;
	int (*func)();
} TestCase[] = {
	{0, "测试连接Memcache服务", &connect_2_memcache_server },
	{0, "测试连接Memcache服务并且验证密码", &connect_to_memcache_server_and_auth_permission},
	{0, "测试连接Memcache服务并且选择缓存实例", &connect_to_memcache_server_and_select_instance},
	{0, "测试执行同步请求SQL语句", &execute_an_query_sql_statement},
	{0, "测试同时执行多条非查询SQL语句", &execute_mutil_sql_statement},
};

int runTestCase()
{
	int i;
	for(i=0; i<sizeof(TestCase)/sizeof(TestCase[0]); i++)
	{
		printf("运行第%d个测试用例：%s\n", i, TestCase[i].name);
		TestCase[i].retval = TestCase[i].func();
		printf("\n");
	}
	return 0;
}

int reportTestCase()
{
	int i=0;
	int succ = 0, fail = 0;

	printf("=================================测试报告=================================\n");
	for(i=0; i<sizeof(TestCase)/sizeof(TestCase[0]); i++)
	{
		if(TestCase[i].retval) {
			fail ++;
		}
		else {
			succ ++;
		}

		printf("第%d个用例测试%s\n", i, TestCase[i].retval?"失败":"成功");
	}

	printf("成功测试 %d 个\n", succ);
	printf("失败测试 %d 个\n", fail);
	printf("总测试数 %d 个\n", fail+succ);
	printf("=================================测试报告=================================\n");

	return 0;
}

int main(int argc, char **argv)
{
	//get_password();
	//get_hostname();

	int l=10000;
	while(l--)
	{
	runTestCase();
	reportTestCase();
	}

	return 0;
}
#endif /*HI_SQL_BUS_DEBUG_MAIN*/
