
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
#include <math.h>

#include "log.h"
#include "util.h"
#include "dbug.h"
#include "sqlbus.h"
#include "config_loader.h"
#include "driver_loader.h"
#include "driver_manager.h"

int main(int argc, const char ** const argv)
{
	DBUG_PUSH("d:t:O");
	DBUG_ENTER(__func__);
	DBUG_PROCESS("main");

	sqlbus_cycle_t cycle = {
		.config = NULL,
		.logger = NULL,
		.sqlbus = NULL,
		.db_type = NULL,
		.db_user = NULL,
		.db_auth = NULL,
		.database = NULL,
		.mem_host = NULL,
		.mem_user = NULL,
		.mem_auth = NULL,
		.mem_database = NULL,
		.config_file = NULL,
	};

	sqlbus_env_init(&cycle, argc, argv);

	daemon(1, 1);

	sqlbus_main(&cycle);

	sqlbus_env_exit(&cycle);

	DBUG_RETURN(RETURN_SUCCESS);
}
