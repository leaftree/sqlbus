
/**
 * a.c
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-06-04 15:40:34
 * Last Modified : 2018-06-04 15:40:34
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef struct mem_info {
	unsigned long long int ptr;
	unsigned int size;
	unsigned int index;
	unsigned int take_back;

	unsigned char file_ask[64];
	unsigned char func_ask[64];
	unsigned char line_ask[64];

	unsigned char file_take[64];
	unsigned char func_take[64];
	unsigned char line_take[64];
}minfo;

int cntinfo = 0;
minfo info[1024000];

unsigned long long int free_size = 0;
unsigned long long int alloc_size = 0;

int main(int argc, char **argv)
{
	char file[512] = "";

	if(argc <= 1) {
		sprintf(file, "%s", "sqlbus.memory.debug.txt");
	} else {
		sprintf(file, "%s", argv[1]);
	}

	if(access(file, F_OK)) {
		fprintf(stderr, "file:%s not exist.\n", file);
		return 1;
	}

	FILE *fp = fopen(file, "r+");
	if(fp == NULL) {
		fprintf(stderr, "open file:%s failed for:%s.\n", file, strerror(errno));
		return 2;
	}

	char value[1024] = "";
	char oper[24] = "";
	char fname[128] = "";
	char line[16] = "";
	char func[64] = "";
	char size[10] = "";
	char ptr[24] = "";
	unsigned int index;

	int lidx=0;
	int flag = 0;
	while(fgets(value, 1024, fp) != NULL)
	{
		lidx++;

		if(strncasecmp(value, "malloc", 6) == 0) {
			int c=sscanf(value, "%s %s %s %s %s %s %d", oper, fname, line, func, size, ptr, &index);
			if(c!=7) {
				fprintf(stderr, "read line(%d) failed\n", lidx);
				return 3;
			}
			flag = 0;
			alloc_size += atoi(size);
		} else {
			int c=sscanf(value, "%s %s %s %s %s %d", oper, fname, line, func, ptr, &index);
			if(c!=6) {
				fprintf(stderr, "read line(%d) failed\n", lidx);
				return 3;
			}
			flag = 1;
		}

		if(flag==0) {
			info[cntinfo].index = index;
			info[cntinfo].take_back = 0;
			info[cntinfo].ptr = (unsigned long long int)strtoull(ptr, NULL, 16);
			info[cntinfo].size = atoi(size);
			sprintf(info[cntinfo].file_ask, "%s", fname);
			sprintf(info[cntinfo].func_ask, "%s", func);
			sprintf(info[cntinfo].line_ask, "%s", line);
			cntinfo ++;
		} else {
			int i=0;
			for(i=0; i<cntinfo; i++) {

				if(info[i].index == index) {
					info[i].take_back = 1;
					sprintf(info[i].func_take, "%s", func);
					sprintf(info[i].file_take, "%s", fname);
					sprintf(info[i].line_take, "%s", line);
					break;
				}
			}

			if(i==cntinfo) {
				fprintf(stderr, "不匹配释放 地址：%s 文件：%s 函数：%s 行号：%s 文件行号：%d 标记：%d\n",
						ptr, fname, func, line, lidx, index);
			}
		}
	}

	fclose(fp);

	int i=0;
	unsigned long int leak = 0;
	for(i=0; i<cntinfo; i++) {
		if(info[i].take_back) {
			free_size += info[i].size;
			continue;
		}

		leak += info[i].size;

		printf("malloc but not free:\n\tfile=%s func=%s line=%s size=%d ptr=%#llx index=%d\n",
				info[i].file_ask, info[i].func_ask, info[i].line_ask, info[i].size, info[i].ptr, info[i].index);
	}

	printf("未释放内存空间：%ld Btye %ld Kbit %ld Mbit\n", leak, leak/1024, leak/1024/1024);
	printf("总共申请：%lld\n", alloc_size);
	printf("总共归还：%lld\n", free_size);
	printf("总共差额：%lld\n", alloc_size-free_size);

	return 0;
}
