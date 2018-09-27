
/**
 * debug.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-09-02 15:23:18
 * Last Modified : 2018-09-02 15:23:18
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <signal.h>
#include <execinfo.h>

void sig_segfault_handler(int sig, siginfo_t *info, void *secret)
{
	int i;
	int nptrs = 0;
	void *trace[100];
	char **symbols;

	nptrs = backtrace(trace, 100);
	symbols = backtrace_symbols(trace, nptrs);
	if(symbols == NULL) {
		return;
	}
	for(i=0; i<nptrs; i++) {
		char *p = index(symbols[i], '(');
		if(p!=NULL) {
			if(*(p+1) != '+') {
				char *q = index(p, '+');
				if(q!=NULL) {
					*q = 0;
					printf("%s->", p+1);
				}
			}
		}
		//printf("%s\n", symbols[i]);
	}
	printf("\n");
	free(symbols);
}
