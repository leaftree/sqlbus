
/**
 * setproctitle.c
 *
 * Copyright (C) 2017 by Liu YunFeng.
 *
 *        Create : 2017-12-28 10:30:26
 * Last Modified : 2017-12-28 10:30:26
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>

#include "log.h"

extern char** environ;

static char *last = NULL;

void set_process_name(char *name)
{
	if(name) {
		prctl(PR_SET_NAME, name);
	}
}

char *get_process_name(char *name)
{
	prctl(PR_GET_NAME, name);
	printf("name=%s\n", name);
	return name;
}

void set_process_title_init(char* argv[])
{
	int i = 0;
	char* tmp = NULL;
	size_t size = 0;

	for(i = 0; environ[i]; i++){
		size += strlen(environ[i]) + 1;
	}

	tmp = malloc(size);
	if(tmp == NULL){
		return ;
	}

	last = argv[0];
	for(i = 0; argv[i]; i++){
		last += strlen(argv[i]) + 1;
	}

	for(i = 0; environ[i]; i++){
		size = strlen(environ[i]) + 1;
		last += size;

		strncpy(tmp, environ[i], size);
		environ[i] = tmp;
		tmp += size;
	}

	(last)--;

	return ;

}

void set_process_title(char* argv[], char* title)
{
	char* tmp = NULL;

	int i=0;
	char buf[1000] = "";

	for(i=0; argv[i]; i++)
	{
		strcat(buf, argv[i]);
		strcat(buf, " ");
	}

	tmp = argv[0];
	strncpy(tmp, title, last - tmp);
	strcat(tmp, " ");
	strcat(tmp, buf);

	/*log_write_stdout("Set process name:%s", tmp);*/

	return ;
}
