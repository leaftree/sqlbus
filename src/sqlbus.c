
/**
 * sqlbus.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-21 15:19:22
 * Last Modified : 2018-03-21 15:19:22
 */

#include <getopt.h>
#include <unistd.h>
#include <libgen.h>
#include <signal.h>
#include <pthread.h>

#include "debug.h"
#include "sqlbus.h"
#include "xmalloc.h"
#include "redisop.h"
#include "request.h"

int gEnvExit = 0;
pthread_t tid;

static int sqlbus_expire_key(sqlbus_cycle_t *cycle);
static int sqlbus_remove_pid_file(sqlbus_cycle_t *cycle);
static int sqlbus_execute_statement(sqlbus_cycle_t *cycle);
static int sqlbus_connect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle);
static int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle);
static int sqlbus_check_if_exit_signal_catched(sqlbus_cycle_t *cycle, int sig);

int sqlbus_check_if_exit_signal_catched(sqlbus_cycle_t *cycle, int sig)
{
	if(!cycle || sig==0) {
		return(RETURN_FAILURE);
	}

	LOG_INFO(cycle->logger, "[sqlbus] Receive a exit signal, server will shutdown now.");

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_main_entry - SQLBUS处理入口
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS:
 */
// TODO: cycle->handle leak memory
int sqlbus_main_entry(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	// 连接到redis
	while(1)
	{
		if(sqlbus_connect_to_memcache(cycle) == RETURN_SUCCESS) {
			break;
		}
		sleep(1);
	}

	// 连接到数据库
	while(1)
	{
		if(sqlbus_connect_to_database(cycle) == RETURN_SUCCESS) {
			break;
		}
		sqlbus_disconnect_to_database(cycle);

		if(sqlbus_check_if_exit_signal_catched(cycle, gEnvExit) == RETURN_SUCCESS) {
			return(RETURN_SUCCESS);
		}
		sleep(1);
	}

	// 循环读取请求，执行请求，回应请求
	while(true)
	{
		if(sqlbus_check_if_exit_signal_catched(cycle, gEnvExit) == RETURN_SUCCESS) {
			break;
		}

		// 从Redis中读取一条请求
		if(sqlbus_recv_from_redis(cycle->sqlbus) != RETURN_SUCCESS)
		{
			//LOG_ERROR(cycle->logger, "[SQLBUS] Waiting redis receive request failure.%s", cycle->sqlbus->errstr);

			while(1)
			{
				if(sqlbus_check_redis_connection(cycle->sqlbus) != RETURN_SUCCESS) {
					sqlbus_disconnect_to_memcache(cycle);
					sqlbus_connect_to_memcache(cycle);
				}
				else {
					break;
				}
				if(sqlbus_check_if_exit_signal_catched(cycle, gEnvExit) == RETURN_SUCCESS) {
					return(RETURN_SUCCESS);
				}
				sleep(1);
			}
			continue;
		}
		LOG_INFO(cycle->logger, "[SQLBUS] Receive request:%s", cycle->sqlbus->request_string);

		// 解析请求串，获取SQL语句，解析失败则丢弃
		if(sqlbus_parse_request(cycle) != RETURN_SUCCESS)
		{
			sqlbus_free_request(cycle);
			LOG_ERROR(cycle->logger, "[SQLBUS] Parse request failure.");
			continue;
		}

		if(DBStmtInitialize(cycle->db.hdbc, &cycle->db.hstmt) != RETURN_SUCCESS)
		{
			sqlbus_free_request(cycle);
			LOG_ERROR(cycle->logger, "[SQLBUS] Initialize sql statement execute info failure.");
			continue;
		}

		sqlbus_execute_statement(cycle);

		// generate response
		if(cycle->sqlbus->sync == SQLBUS_SYNC) {
			sqlbus_generate_response(cycle);
			sqlbus_write_to_redis(cycle->sqlbus);
			LOG_DEBUG(cycle->logger, "[SQLBUS] response json string:%s", cycle->sqlbus->response_string);
			sqlbus_expire_key(cycle);
		}
		sqlbus_free_request(cycle);

		if(DBStmtFinalize(cycle->db.hstmt) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
		}
		cycle->db.hstmt = NULL;

		LOG_INFO(cycle->logger, "[SQLBUS] DB operation finished.");
	}

	sqlbus_disconnect_to_database(cycle);
	sqlbus_disconnect_to_memcache(cycle);

	return(RETURN_SUCCESS);
}

static int sqlbus_chg_work_dir(sqlbus_cycle_t *cycle)
{
	char path[512] = "";

	if(get_config_value(cycle->config, "DEFAULT", "chdir", path) == RETURN_SUCCESS) {
		return chdir(path);
	}
	return(RETURN_FAILURE);
}

/**
 * sqlbus_env_log_init - 初始化日志功能
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 日志打开失败
 *  RETURN_SUCCESS: 日志打开成功
 */
static int sqlbus_env_log_init(sqlbus_cycle_t *cycle)
{
	static char level[64] = "INFO";
	static char trace[64] = "NO";
	static char catalog[128] = "";
	static char logfile[128] = "";
	static char feedline[64] = "YES";
	static char crontime[32] = "00:00:00";
	static char maxsizes[32] = "20mb";

	cycle->logger->stdio = 0;
	sprintf(catalog, "%s", defaultLogPathName);
	sprintf(logfile, "%s", defaultLogFileName);

	get_config_value(cycle->config, "LOG", "Level", level);
	get_config_value(cycle->config, "LOG", "Trace", trace);
	get_config_value(cycle->config, "LOG", "Catalog", catalog);
	get_config_value(cycle->config, "LOG", "MaxSize", maxsizes);
	get_config_value(cycle->config, "LOG", "FileName", logfile);
	get_config_value(cycle->config, "LOG", "RolateCron", crontime);
	get_config_value(cycle->config, "LOG", "SpaceLineAfterLog", feedline);

	if(!strcasecmp(trace, "YES") || !strcasecmp(trace, "ON")) {
		cycle->logger->trace = LOG_OPTION_ON;
	} else {
		cycle->logger->trace = LOG_OPTION_OFF;
	}

	if(!strcasecmp(feedline, "YES") || !strcasecmp(feedline, "ON")) {
		cycle->logger->feed = LOG_OPTION_ON;
	} else {
		cycle->logger->feed = LOG_OPTION_OFF;
	}

	cycle->logger->cron = log_parse_crontime(crontime);
	cycle->logger->level = log_level_string_to_type(level);
	cycle->logger->maxsize = log_parse_maxsize(maxsizes);

	if(cycle->envs.daemonize != 1) {
		sprintf(catalog, "%s", "/dev");
		sprintf(logfile, "%s", "stdout");
		cycle->logger->stdio = 1;
	}

	if(cycle->logger->feed != LOG_OPTION_ON && cycle->envs.debug) {
		cycle->logger->trace = LOG_OPTION_ON;
	}

	if(log_open(catalog, logfile, cycle->logger) != RETURN_SUCCESS) {
		log_write_stderr("Open log file[%s/%s] failure.%s", catalog, logfile, strerror(errno));
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

static int sqlbus_env_database_init(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->config) {
		return(RETURN_FAILURE);
	}

	char db[64] = "";
	char user[64] = "";
	char auth[64] = "";
	char inst[64] = "";

	get_config_value(cycle->config, "default", "database", db);

	if(!*db) {
		LOG_ERROR(cycle->logger, "Not found key[Database] in section \"Default\"");
		return(RETURN_FAILURE);
	}

	if(check_config_is_section_exist(cycle->config, db) != RETURN_SUCCESS) {
		LOG_ERROR(cycle->logger, "Not found section[%s] in config file.", db);
		return(RETURN_FAILURE);
	}

	get_config_value(cycle->config, db, "username", user);
	get_config_value(cycle->config, db, "password", auth);
	get_config_value(cycle->config, db, "database", inst);

	cycle->db.type = xStrdup(db);
	cycle->db.user = xStrdup(user);
	cycle->db.auth = xStrdup(auth);
	cycle->db.database = xStrdup(inst);

	return(RETURN_SUCCESS);
}

static void sqlbus_env_database_free(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return;
	}

	xFree(cycle->db.type);
	xFree(cycle->db.host);
	xFree(cycle->db.user);
	xFree(cycle->db.auth);
	xFree(cycle->db.database);
	return;
}

static int sqlbus_env_memcache_init(sqlbus_cycle_t *cycle)
{
	char auth[64] = "";
	char port[64] = "6379";
	char host[64] = "127.0.0.1";
	char database[64] = "0";
	char memcache[64] = "redis";
	char resp_timeout[64] = "";
	char conn_timeout[64] = "5";

	if(!cycle || !cycle->config) {
		return(RETURN_FAILURE);
	}

	get_config_value(cycle->config, "default", "memcache", memcache);

	if(check_config_is_section_exist(cycle->config, memcache) != RETURN_SUCCESS) {
		LOG_ERROR(cycle->logger, "Not found section[%s] in config file.", memcache);
		return(RETURN_FAILURE);
	}

	get_config_value(cycle->config, memcache, "host", host);
	get_config_value(cycle->config, memcache, "port", port);
	get_config_value(cycle->config, memcache, "password", auth);
	get_config_value(cycle->config, memcache, "database", database);
	get_config_value(cycle->config, memcache, "ConnectTimeout", conn_timeout);

	cycle->memcache.host = xStrdup(host);
	cycle->memcache.auth = xStrdup(auth);
	cycle->memcache.database = xStrdup(database);
	cycle->memcache.port = atoi(port);
	cycle->memcache.ctimeo = atoi(conn_timeout);

	if(RETURN_SUCCESS == get_config_value(cycle->config,
				memcache,
				"ResponseTimeout",
				resp_timeout)) {
		cycle->memcache.rtimeo = atoi(resp_timeout);
	}

	return(RETURN_SUCCESS);
}

static void sqlbus_env_memcache_free(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return;
	}

	cycle->memcache.port = 0;
	cycle->memcache.ctimeo = 0;
	xFree(cycle->memcache.auth);
	xFree(cycle->memcache.host);
	xFree(cycle->memcache.user);
	xFree(cycle->memcache.database);

	return;
}

static int sqlbus_get_background_running_config(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->config) {
		return(RETURN_FAILURE);
	}

	char daemonize[64] = "";

	get_config_value(cycle->config, "default", "daemonize", daemonize);

	if(*daemonize != 0 && (!strcmp(daemonize, "0") || !strcmp(daemonize, "1"))) {
		cycle->envs.daemonize = atoi(daemonize);
		return(RETURN_SUCCESS);
	}

	return(RETURN_FAILURE);
}

/**
 * sqlbus_config_init - 加载sqlbus配置
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 加载成功
 *  RETURN_SUCCESS: 加载失败
 */
int sqlbus_config_init(sqlbus_cycle_t *cycle)
{
	static char config_file[128] = "/etc/sqlbus.ini";

	if(!cycle) {
		return(RETURN_FAILURE);
	}

	cycle->config = xMalloc(sizeof(sqlbus_config_t));
	cycle->config->section = NULL;

	if(cycle->envs.config_file == NULL)
		cycle->envs.config_file = xStrdup(config_file);

	if(load_config(cycle->envs.config_file, cycle->config) != RETURN_SUCCESS) {
		log_write_stderr("Load configuration file[%s] failure.[%s]", cycle->envs.config_file, strerror(errno));

		xFree(cycle->envs.config_file);
		xFree(cycle->config);
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_env_init - 初始化sqlbus运行环境
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者初始化资源错误
 *  RETURN_SUCCESS: 初始化成功
 */
int sqlbus_env_init(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	cycle->logger = xMalloc(sizeof(sqlbus_log_t));
	cycle->sqlbus = xMalloc(sizeof(sqlbus_handle_t));

	if(cycle->config == NULL || cycle->logger == NULL || cycle->sqlbus == NULL) {
		// can not go to error_exit flag
		xFree(cycle->logger);
		xFree(cycle->sqlbus);
		log_write_stderr("%s", "Allocate memory failure.");
		return(RETURN_FAILURE);
	}

	sqlbus_chg_work_dir(cycle);

	if(sqlbus_env_log_init(cycle) != RETURN_SUCCESS) {
		goto error_exit;
	}

	if(sqlbus_create_pid_file(cycle) != RETURN_SUCCESS) {
		goto error_exit;
	}

	if(sqlbus_env_database_init(cycle) != RETURN_SUCCESS) {
		goto error_exit;
	}

	if(sqlbus_env_memcache_init(cycle) != RETURN_SUCCESS) {
		goto error_exit;
	}

	sqlbus_get_background_running_config(cycle);

	cycle->sqlbus->recv_channel = xStrdup(defaultQueue);
	cycle->sqlbus->oper_timeout = defaultOperTimeOut;

	return(RETURN_SUCCESS);

error_exit:
	log_close(cycle->logger);
	sqlbus_env_database_free(cycle);
	sqlbus_env_memcache_free(cycle);
	sqlbus_remove_pid_file(cycle);

	xFree(cycle->envs.pid_file);
	xFree(cycle->sqlbus->recv_channel);

	xFree(cycle->logger);
	xFree(cycle->sqlbus);
	return(RETURN_FAILURE);
}

/**
 * sqlbus_env_exit - 退出SQLBUS服务
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_env_exit(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	unload_config(cycle->config);
	sqlbus_env_database_free(cycle);
	sqlbus_env_memcache_free(cycle);
	sqlbus_remove_pid_file(cycle);

	log_close(cycle->logger);

	xFree(cycle->envs.pid_file);
	xFree(cycle->envs.config_file);

	xFree(cycle->sqlbus->recv_channel);

	xFree(cycle->sqlbus);
	xFree(cycle->logger);
	xFree(cycle->config);

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_connect_to_database - 连接到数据库
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者连接失败
 *  RETURN_SUCCESS: 连接成功
 */
int sqlbus_connect_to_database(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	if(DBEnvInitialize(&cycle->db.henv, cycle->envs.config_file) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db environment failure.");
		return(RETURN_FAILURE);
	}

	if(DBConnectInitialize(cycle->db.henv, &cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Initialize db connection resource failure.");
		return(RETURN_FAILURE);
	}

	LOG_INFO(cycle->logger, "[SQLBUS] Try to connect to database[%s]", cycle->db.type);

	// TODO:show database driver info
	LOG_INFO(cycle->logger, "[SQLBUS] Database connection: Type[%s] user[%s] auth[%s] database[%s]",
			cycle->db.type, cycle->db.user, cycle->db.auth, cycle->db.database);

	if(DBConnect(cycle->db.hdbc, cycle->db.type, cycle->db.user, cycle->db.auth,
				cycle->db.host, cycle->db.database, cycle->db.port) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Connecting to database failure: %s",
				cycle->db.hdbc->error->errstr);

		return(RETURN_FAILURE);
	}

	LOG_INFO(cycle->logger, "[SQLBUS] Connect to database[%s] success.", cycle->db.type);

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_disconnect_to_database - 断开与数据库的连接
 *
 * @cycle: sqlbus主体循环接口
 *
 * @sqlbus_disconnect_to_database会释放所有与数据库通讯有关的资源
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 释放成功
 */
int sqlbus_disconnect_to_database(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->db.hdbc) {
		return(RETURN_FAILURE);
	}

	/*
		 if(cycle->db.hdbc->conn_status == SQLBUS_DB_CONNECTION_YES &&
		 DBDisconnect(cycle->db.hdbc) != RETURN_SUCCESS)
		 */
	if(DBDisconnect(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Logout database failure.");
		if(DBGetErrorMessage(cycle->db.hdbc, SQLBUS_HANDLE_DBC) == RETURN_SUCCESS)
			LOG_ERROR(cycle->logger, "%s.", cycle->db.hdbc->error->errstr);
		//return(RETURN_FAILURE);
	}

	if(DBConnectFinalize(cycle->db.hdbc) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database connection resource failure.");
		//return(RETURN_FAILURE);
	}

	if(DBEnvFinalize(cycle->db.henv) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Release database environment resource failure.");
		return(RETURN_FAILURE);
	}
	cycle->db.hdbc = NULL;
	cycle->db.henv = NULL;

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_connect_to_memcache - 连接到缓存
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效或者连接、认证失败
 *  RETURN_SUCCESS: 连接成功
 */
int sqlbus_connect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle) {
		return(RETURN_FAILURE);
	}

	char *nullString(char *str) {
		if(str==NULL || *str==0x0) {
			return("(null)");
		}
		return(str);
	}

	/**
	 * Connection
	 */
	LOG_INFO(cycle->logger,
			"[SQLBUS] Memcache connection: host[%s] auth[%s] port[%d] database[%s] timeout[%d]",
			cycle->memcache.host, nullString(cycle->memcache.auth), cycle->memcache.port, cycle->memcache.database, cycle->memcache.ctimeo);

	errno=0;
	cycle->sqlbus->redis = redis_connection(cycle->memcache.host, cycle->memcache.port, cycle->memcache.ctimeo);
	if(cycle->sqlbus->redis == NULL) {
		LOG_ERROR(cycle->logger, "[SQLBUS] Connect to redis failure: %s", strerror(errno));
		return(RETURN_FAILURE);
	}

	/**
	 * Authenticate
	 */
	if(cycle->memcache.auth) {
		if(redis_auth(cycle->sqlbus->redis, cycle->memcache.auth) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Memcache authentication failure.");
			return(RETURN_FAILURE);
		}
	}

	/**
	 * Select database
	 */
	if(cycle->memcache.database) {
		if(redis_select(cycle->sqlbus->redis, atoi(cycle->memcache.database)) != RETURN_SUCCESS) {
			LOG_ERROR(cycle->logger, "[SQLBUS] Memcahce select database failure.");
			//return(RETURN_FAILURE);
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_disconnect_to_memcache - 断开缓存的连接
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_disconnect_to_memcache(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->sqlbus->redis) {
		return(RETURN_FAILURE);
	}

	redis_logout(cycle->sqlbus->redis);
	cycle->sqlbus->redis = NULL;

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_parse_env_cmd_args - 解析命令行参数
 *
 * @cycle: sqlbus主体循环接口
 * @argc: 命令行参数数量
 * @argv: 命令行参数
 *
 * return value:
 *  RETURN_FAILURE: 解析失败
 *  RETURN_SUCCESS: 解析成功
 */
int sqlbus_parse_env_cmd_args(sqlbus_cycle_t *cycle, int argc, const char *const argv[])
{
	if(!cycle || (argc && !argv)) {
		return(RETURN_FAILURE);
	}
	if(!argc) {
		return(RETURN_SUCCESS);
	}

	int opt;
	static char shortoptions[] = "bc:fdhk";
	static struct option longoptions[] = {
		{"file",       1, NULL, 'c'},
		{"help",       0, NULL, 'h'},
		{"kill",       0, NULL, 'k'},
		{"debug",      0, NULL, 'd'},
		{"config",     1, NULL, 'c'},
		{"pidfile",    1, NULL, 'p'},
		{"daemonize",  0, NULL, 'b'},
		{"background", 0, NULL, 'b'},
		{"foreground", 0, NULL, 'f'},
		{NULL,         0, NULL, 0},
	};

	void dialog() {
		fprintf(stderr, "%s",
				"\e[1;37mHelp manual...\n"
				"    -h|--help                     Dialog this helper\n"
				"    -k|--kill                     Kill the service process\n"
				"    -d|--debug                    Enable debugging to show source code\n"
				"    -p|--pidfile                  Specify the file to save pid of master process\n"
				"    -c|--config|--file            Specify the configuration file\n"
				"    -f|--background               Make sqlbus run in terminal\n"
				"    -b|--daemonize|--background   Make sqlbus run as a daemon process\n"
				);
		exit(0);
	}

	while((opt=getopt_long(argc, (char*const*)argv, shortoptions, longoptions, NULL)) != -1)
	{
		switch(opt)
		{
			case 'c':
				xFree(cycle->envs.config_file);
				cycle->envs.config_file = xStrdup(optarg);
				break;

			case 'd':
				cycle->envs.debug = 1;
				break;

			case 'f':
				cycle->envs.daemonize = -1;
				break;

			case 'b':
				if(cycle->envs.daemonize != -1)
					cycle->envs.daemonize = 1;
				break;

			case 'p':
				xFree(cycle->envs.pid_file);
				cycle->envs.pid_file = xStrdup(optarg);
				break;

			case 'h':
				dialog();
				break;

			case '?':
			default:
				return(RETURN_FAILURE);
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_create_pid_file - 创建pid文件，用于标记进程id
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 创建失败
 *  RETURN_SUCCESS: 创建成功
 */
int sqlbus_create_pid_file(sqlbus_cycle_t *cycle)
{
	char pid[16] = "";
	char pid_file[512] = defaultServicePidFile;

	if(!cycle) {
		return(RETURN_FAILURE);
	}

	if(cycle->config) {
		get_config_value(cycle->config, "default", "pidfile", pid_file);
	}

	if(cycle->envs.pid_file) {
		sprintf(pid_file, "%s", cycle->envs.pid_file);
	} else {
		cycle->envs.pid_file = xStrdup(pid_file);
	}

	char pfile[512] = "";
	sprintf(pfile, "%s", pid_file);
	char *pf = dirname(pfile);
	if(access(pf, F_OK)!=0) {
		if(mkdir(pf, 0774)!=0) {
			log_write_stderr("Create dir:%s, %s", pf, strerror(errno));
			return(RETURN_FAILURE);
		}
	}

	int fd = open(pid_file, O_WRONLY|O_TRUNC|O_CREAT, 0664);
	if(fd<0) {
		xFree(cycle->envs.pid_file);
		log_write_stderr("Create pid file[%s] failure.%s", pid_file, strerror(errno));
		return(RETURN_FAILURE);
	}

	sprintf(pid, "%d\n", getpid());
	write(fd, pid, strlen(pid));
	close(fd);

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_remove_pid_file - 移除pid文件
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 移除失败
 *  RETURN_SUCCESS: 移除成功
 */
int sqlbus_remove_pid_file(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->envs.pid_file) {
		return(RETURN_FAILURE);
	}

	if(!access(cycle->envs.pid_file, F_OK)) {
		if(!remove(cycle->envs.pid_file)) {
			return(RETURN_FAILURE);
		}
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_expire_key - 设置键值的有效期
 *
 * @cycle: sqlbus主体循环接口
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 成功
 */
int sqlbus_expire_key(sqlbus_cycle_t *cycle)
{
	int retval;

	sqlbus_handle_t *handle = NULL;

	if(!cycle || !cycle->sqlbus) {
		return(RETURN_FAILURE);
	}

	handle = cycle->sqlbus;
	retval = redis_expire(handle->redis, handle->send_channel, cycle->memcache.rtimeo);

	if(retval == RETURN_FAILURE) {
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}

/**
 * sqlbus_generate_response - 创建sqlbus请求回应数据
 *
 * @cycle: sqlbus主体循环接口
 *
 * @sqlbus_generate_response会根据数据库操纵结果失败回应消息
 *
 * return value:
 *  RETURN_FAILURE: 参数无效
 *  RETURN_SUCCESS: 回应数据创建成功
 */
int sqlbus_generate_response(sqlbus_cycle_t *cycle)
{
	if(!cycle || !cycle->sqlbus || !cycle->db.hstmt) {
		return(RETURN_FAILURE);
	}

	int i, j;
	int errid = 0;
	int rows = 0;
	int fields = 0;
	char field[128] = "";
	char value[10000] = "";

	HSTMT hstmt = cycle->db.hstmt;
	HSQLBUS handle = cycle->sqlbus;

	cJSON *root = NULL, *leaf = NULL, *array = NULL, *farray = NULL;

	// json root node
	root = cJSON_CreateObject();

	/** row counter */
	if(DBGetRowCount(hstmt, &rows) != RETURN_SUCCESS) { }

	/** Field counter */
	if(DBGetFieldCount(hstmt, &fields) != RETURN_SUCCESS) { }

	/** type */
	leaf = cJSON_CreateString("response");
	cJSON_AddItemToObject(root, "type", leaf);

	/** App */
	leaf = cJSON_CreateString("SQLBUS-SERVER");
	cJSON_AddItemToObject(root, "app", leaf);

	/** pid */
	leaf = cJSON_CreateNumber(getpid());
	cJSON_AddItemToObject(root, "pid", leaf);

	/** database type */
	leaf = cJSON_CreateString(cycle->db.type);
	cJSON_AddItemToObject(root, "dbtype", leaf);

	/** timestamp */
	leaf = cJSON_CreateNumber(time(NULL));
	cJSON_AddItemToObject(root, "timestamp", leaf);

	/** uuId */
	leaf = cJSON_CreateString(handle->uuid);
	cJSON_AddItemToObject(root, "uuid", leaf);

	/** message */
	if(hstmt->result_code == SQLBUS_DB_EXEC_RESULT_FAIL) {
		leaf = cJSON_CreateString(hstmt->error->errstr);
		//leaf = cJSON_CreateString("Sql statement run failed");
	} else if(hstmt->result_code == SQLBUS_DB_UNIQUE_CONSTRAINT) {
		leaf = cJSON_CreateString("Duplicate records");
	} else if(hstmt->result_code ==	SQLBUS_DB_DATA_NOT_FOUND) {
		leaf = cJSON_CreateString("Record not found");
	} else {
		leaf = cJSON_CreateString("");
	}
	cJSON_AddItemToObject(root, "message", leaf);

	/** errorid */
	if(hstmt->result_code == SQLBUS_DB_EXEC_RESULT_SUCC){
		errid = RETURN_SUCCESS;
	} else if(hstmt->result_code == SQLBUS_DB_DATA_NOT_FOUND) {
		errid = SQLBUS_DB_DATA_NOT_FOUND;
	} else {
		errid = RETURN_FAILURE;
	}
	leaf = cJSON_CreateNumber(errid);
	cJSON_AddItemToObject(root, "errorid", leaf);

	leaf = cJSON_CreateNumber(fields);
	cJSON_AddItemToObject(root, "fields", leaf);

	array = cJSON_CreateArray();
	cJSON_AddItemToObject(root, "field", array);
	for(i=0; i<fields; i++)
	{
		if(RETURN_SUCCESS != DBGetFieldNameIdx(hstmt, i, field)) {
			goto fail;
		}

		leaf = cJSON_CreateString(field);
		if(leaf == NULL) {
			goto fail;
		}
		cJSON_AddItemToArray(array, leaf);
	}

	leaf = cJSON_CreateNumber(rows);
	cJSON_AddItemToObject(root, "rows", leaf);

	/** row result set */
	array = cJSON_CreateArray();
	for(i=0; i<rows; i++)
	{
		farray = cJSON_CreateArray();
		cJSON_AddItemToArray(array, farray);
		DBGetNextRow(hstmt);

		for(j=0; j<fields; j++)
		{
			if(RETURN_SUCCESS != DBGetFieldValueIdx(hstmt, i, j, value)) {
				goto fail;
			}

			leaf = cJSON_CreateString(value);
			if(leaf == NULL) {
				goto fail;
			}
			cJSON_AddItemToArray(farray, leaf);
		}
	}
	cJSON_AddItemToObject(root, "result", array);

	char *resp = cJSON_PrintUnformatted(root);
	if(resp == NULL) {
		goto fail;
	}
	handle->response_string = xStrdup(resp);

	cJSON_free(resp);
	cJSON_Delete(root);

	return(RETURN_SUCCESS);

fail:
	cJSON_free(resp);
	cJSON_Delete(root);
	return(RETURN_FAILURE);
}

int sqlbus_execute_statement(sqlbus_cycle_t *cycle)
{
	if(cycle->sqlbus->statement.type == 1)
	{
		LOG_DEBUG(cycle->logger, "[SQLBUS] single sql statement:%s", cycle->sqlbus->statement.data);
		// 单个SQL语句，执行过后返回
		if(DBExecute(cycle->db.hstmt, cycle->sqlbus->statement.data) == RETURN_SUCCESS)
		{
			return(RETURN_SUCCESS);
		}
		LOG_ERROR(cycle->logger, "[SQLBUS] Execute a sql statement failure");
		DBGetErrorMessage(cycle->db.hstmt, SQLBUS_HANDLE_STMT);
		LOG_ERROR(cycle->logger, "%s", cycle->db.hstmt->error->errstr);
	}
	else
	{
		LOG_DEBUG(cycle->logger, "[SQLBUS] double sql statement:%s / %s", cycle->sqlbus->statement.ui.update, cycle->sqlbus->statement.ui.insert);

		// 两个SQL(Update/Insert)，当Update执行返回不存在记录时执行Insert语句
		if(DBExecute(cycle->db.hstmt, cycle->sqlbus->statement.ui.update) != RETURN_SUCCESS)
		{
			DBGetErrorMessage(cycle->db.hstmt, SQLBUS_HANDLE_STMT);
			LOG_WARN(cycle->logger, "[SQLBUS] Executed update statement failure.");
			LOG_ERROR(cycle->logger, "%s", cycle->db.hstmt->error->errstr);

			if(cycle->db.hstmt->result_code == SQLBUS_DB_DATA_NOT_FOUND)
			{
				LOG_WARN(cycle->logger, "[SQLBUS] Record not found, executed update statement unsuccess. it'll executes insert statement.");

				if(DBExecute(cycle->db.hstmt, cycle->sqlbus->statement.ui.insert) == RETURN_SUCCESS)
				{
					LOG_INFO(cycle->logger, "[SQLBUS] Insert Sql do successed.");
					return(RETURN_SUCCESS);
				}
				DBGetErrorMessage(cycle->db.hstmt, SQLBUS_HANDLE_STMT);
				LOG_ERROR(cycle->logger, "[SQLBUS] Executed insert statement failure.");
				LOG_ERROR(cycle->logger, "%s", cycle->db.hstmt->error->errstr);
			}
		}
	}

	// 检查数据库连接状态，连接正常则表示SQL语句执行失败，返回错误
	if(SQLBUS_CHECK_DB_CONNECTION(cycle->db.hstmt->connection))
	{
		return(RETURN_FAILURE);
	}

	LOG_WARN(cycle->logger, "[SQLBUS] database not connected");

	if(DBStmtFinalize(cycle->db.hstmt) != RETURN_SUCCESS) {
		LOG_ERROR(cycle->logger, "[SQLBUS] Release sql statement info failure.");
	}
	cycle->db.hstmt = NULL;

	if(sqlbus_write_back_to_redis(cycle->sqlbus) != RETURN_SUCCESS)
	{
		LOG_ERROR(cycle->logger, "[SQLBUS] Write back request failure.");
		LOG_ERROR(cycle->logger, "%s.", cycle->sqlbus->errstr);
	}

	// 写回redis后要释放请求数据的资源
	sqlbus_free_request(cycle);

	// 尝试重新连接数据库
	while(1)
	{
		// 释放以上执行SQL时占用的资源
		sqlbus_disconnect_to_database(cycle);

		if(sqlbus_connect_to_database(cycle) == RETURN_SUCCESS) {
			LOG_INFO(cycle->logger, "[SQLBUS] reconnected to database.");
			break;
		}
		if(sqlbus_check_if_exit_signal_catched(cycle, gEnvExit) == RETURN_SUCCESS) {
			return(RETURN_SUCCESS);
		}
		sleep(1);
	}

	return(RETURN_SUCCESS);
}

int sqlbus_serviced_status_check(sqlbus_cycle_t *cycle)
{
	int pid = -1;
	char file[512] = defaultServicePidFile;

	if(readpid(file, &pid)==RETURN_SUCCESS) {
		if(match_prog_by_pid("sqlbus", pid)==0) {
			return(RETURN_SUCCESS);
		}
	}

	if(get_config_value(cycle->config,
				"default", "pidfile", file)==RETURN_SUCCESS){
		if(readpid(file, &pid)==RETURN_SUCCESS) {
			if(match_prog_by_pid("sqlbus", pid)==0) {
				return(RETURN_SUCCESS);
			}
		}
	}

	return(RETURN_FAILURE);
}

void sqlbus_signal_exit(int no)
{
	char *strsig(int no) {
		char *sig[] = {
			[SIGINT ] = "int",
			[SIGQUIT] = "quit",
			[SIGTERM] = "term",
		};
		return sig[no];
	}

  gEnvExit = 1;
  log_write_stdout("sqlbus recv %s-signal, wait to exit", strsig(no));

	pthread_cancel(tid);

  return;
}

int sqlbus_register_signal()
{
	signal(SIGINT,  sqlbus_signal_exit);
	signal(SIGQUIT, sqlbus_signal_exit);
	signal(SIGTERM, sqlbus_signal_exit);

	struct sigaction act = {
		.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO,
		.sa_sigaction = sig_segfault_handler,
	};
	sigemptyset(&act.sa_mask);
	sigaction(SIGSEGV, &act, NULL);

	return(RETURN_SUCCESS);
}

int sqlbus_start_log_wathcer_thread(sqlbus_cycle_t *cycle)
{
	//pthread_t tid;

	if(!cycle || !cycle->logger) {
		return(RETURN_FAILURE);
	}

	pthread_mutex_init(&cycle->logger->mutex, NULL);

	if(pthread_create(&tid, NULL, log_op_watcher_loop, (void*)cycle->logger) !=0 ) {
		LOG_ERROR(cycle->logger, "[sqlbus] pthread_create() failed for: %s", strerror(errno));
		return(RETURN_FAILURE);
	}

	if(pthread_detach(tid) != 0) {
		LOG_ERROR(cycle->logger, "[sqlbus] pthread_detach() failed for: %s", strerror(errno));
		return(RETURN_FAILURE);
	}

	return(RETURN_SUCCESS);
}
