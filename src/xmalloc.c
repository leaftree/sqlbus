
/**
 * xmalloc.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-15 01:52:49
 * Last Modified : 2018-04-15 01:52:49
 */

#include <malloc.h>
#include <string.h>

#ifdef MEM_DEBUG_ON

static long long unsigned int cnt = 0;
const char mem_debug_file[] = "sqlbus.memory.debug.txt";

void *_afc_malloc(int size, const char *file, const char *func, int line)
{
	printf("malloc\n");
	size+=8;

	void *ptr = malloc(size);
	if(ptr == NULL)
		return(NULL);

	memset(ptr, 0x0, size);

	*((long long unsigned int*)ptr) = cnt++;

	FILE *fp = fopen(mem_debug_file, "a+");
	if(fp==NULL) {
		fprintf(stderr, "fopen file[%s] error when debug memory malloc\n", mem_debug_file);
		return ptr;
	}
	char info[4092] = "";
	int byte=sprintf(info, "malloc %-25s %03d %-25s %04d %p %llu\n",
			file, line, func, size, ptr, *(long long unsigned int*)ptr);
	fwrite(info, byte, 1, fp);
	fclose(fp);

	return ptr+8;
}

char *_afc_strdup(char *str, const char *file, const char *func, int line)
{
	if(str == NULL) {
		return NULL;
	}

	char *new = _afc_malloc(strlen(str)+1, file, func, line);
	if(!new) {
		return NULL;
	}
	memcpy(new, str, strlen(str));

	return new;
}

void _afc_free(void *pmem, const char *file, const char *func, int line)
{
	if(pmem == NULL) {
		return;
	}
	printf("free\n");

	pmem-=8;
	FILE *fp = fopen(mem_debug_file, "a+");
	if(fp==NULL) {
		fprintf(stderr, "fopen file[%s] error when debug memory malloc\n", mem_debug_file);
		free(pmem);
		return;
	}
	char info[4092] = "";
	int byte=sprintf(info, "  free %-25s %03d %-25s      %p %llu\n",
			file, line, func, pmem , *(unsigned long long int*)pmem);
	fwrite(info, byte, 1, fp);
	fclose(fp);

	free(pmem);

	return;
}
#endif
