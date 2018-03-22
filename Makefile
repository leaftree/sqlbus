

#================================================================
#   Copyright (C) 2018 Liu YunFeng. All rights reserved.
#   
#   文件名称：Makefile
#   创 建 者：Liu YunFeng
#   创建日期：2018年03月10日
#   描    述：
#
#================================================================

CC = gcc
CFLAG = -g -O3 -Wall

THIRD_INC = -I./third/cJSON -I./third/dbug

OBJECT = main.o log.o util.o driver_loader.o config_loader.o driver_manager.o sqlbus.o redisop.o

LNKLIBS = -L/usr/lib -lhiredis -L./library -lcjson -ldl

ifeq ($(ver), debug)
CFLAGS += $(CFLAG) -g3 -DDBUG_ON
LNKLIBS += -ldbug
else
CFLAGS += $(CFLAG)
endif

all:sqlbus

sqlbus:$(OBJECT)
	$(CC) $(OBJECT) $(CFLAGS) $(LNKLIBS) -o $@ $(THIRD_INC)

.c.o:
	$(CC) $(CFLAGS) -I. -c $< $(THIRD_INC)

.PHONY:clean
clean:
	rm -f $(OBJECT) sqlbus app.log
