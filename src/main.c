
/**
 * a.cpp
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-10 18:57:59
 * Last Modified : 2018-03-10 18:57:59
 */

#include <stdlib.h>
#include <string.h>

#include "sqlbus.h"
#include "setproctitle.h"

int main(int argc, char **argv, char **env)
{
	sqlbus_cycle_t cycle = {
		.config = NULL,
		.logger = NULL,
		.sqlbus = NULL,

		.envs = {
			.debug = 0,
			.pid_file = NULL,
			.config_file = NULL,
			.daemonize = 0,
		},

		.db = {
			.port = 0,
			.type = NULL,
			.user = NULL,
			.auth = NULL,
			.host = NULL,
			.database = NULL,
		},
		.memcache = {
			.port = 6379,
			.host = NULL,
			.user = NULL,
			.auth = NULL,
			.database = NULL,
			.rtimeo = defaultExpireTime,
		},
	};

	// parse app parameters
	if(sqlbus_parse_env_cmd_args(&cycle, argc, (const char *const*)argv) != RETURN_SUCCESS) {
		log_write_stderr("Parse environment command arguments failure.");
		return(RETURN_FAILURE);
	}

	// Load configuration for sqlbus from config file
	if(sqlbus_config_init(&cycle)!=RETURN_SUCCESS){
		log_write_stderr("Load configurations failure.");
		return(RETURN_FAILURE);
	}

	// check sqlbus is serviced or not
	if(sqlbus_serviced_status_check(&cycle)==RETURN_SUCCESS) {
		log_write_stderr("Sqlbus server was started, should not be started again");
		return(RETURN_SUCCESS);
	}

	log_write_stdout("Starting sqlbus server");

	// Make it run in the back-ground
	if(cycle.envs.daemonize == 1) {
		daemon(1, 1);
	}

	// init run time env
	if(sqlbus_env_init(&cycle) != RETURN_SUCCESS)
	{
		log_write_stderr("Init run time env");
		return EXIT_FAILURE;
	}
	log_write_stdout("Init run time env");

	// create main process pid
	if(sqlbus_create_pid_file(&cycle) != RETURN_SUCCESS) {
		log_write_stderr("Create pid file[%s] failure.%s", cycle.envs.pid_file, strerror(errno));
		return EXIT_FAILURE;
	}

	// Start log file watcher thread, it works for rorate log file periodically,
	// and it will re-create the log file when someone deleted.
	//
	// If service run in terminal, log file is /dev/stdout, so we do not watcher
	// the file which HaiPa delete by someone who HuaiRen
	if(cycle.envs.daemonize == 1) {
		sqlbus_start_log_wathcer_thread(&cycle);
	}

	// register os signal
	sqlbus_register_signal();

	// set process title
	set_process_title_init(argv);
	set_process_title(argv, SQLBUS_MASTER_TITLE);

	// Setup process name
	set_process_name(SQLBUS_MASTER_NAME);

	// change euid
	if(getuid() == 0) {
		if(setuid(65534)<0) {
			log_write_stderr("Change uid failed.");
		}
	}

	log_write_stdout("Sqlbus server started");

	// loop main
	sqlbus_main_entry(&cycle);

	LOG_INFO(cycle.logger, "[SQLBUS] server stopped");
	log_write_stdout("Sqlbus server stoped & exited");

	// free resource
	sqlbus_env_exit(&cycle);

	return(EXIT_SUCCESS);
}
