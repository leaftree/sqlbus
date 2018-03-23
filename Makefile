

#================================================================
#   Copyright (C) 2018 Liu YunFeng. All rights reserved.
#   
#   文件名称：Makefile
#   创 建 者：Liu YunFeng
#   创建日期：2018年03月10日
#   描    述：
#
#================================================================

CCCOLOR   = "\033[33m"
LINKCOLOR = "\033[34;1m"
SRCCOLOR  = "\033[31m"
RMCOLOR   = "\033[1;31m"
BINCOLOR  = "\033[37;1m"
MAKECOLOR = "\033[32;1m"
ENDCOLOR  = "\033[0m"

QUIET_CC  = @printf '%b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_RM  = @printf '%b %b\n' $(LINKCOLOR)REMOVE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_EXE = @printf '%b %b\n' $(LINKCOLOR)EXEC$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LNK = @printf '%b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;

QUIET_PROC=$(QUIET_CC)$(CC)
QUIET_EXEC=$(QUIET_EXE)exec
QUIET_REMOVE=$(QUIET_RM)rm -f
QUIET_LINK=$(QUIET_LNK)gcc

CC = gcc
CFLAG = -g -O3 -Wall

THIRD_INC = -I./third/cJSON -I./third/dbug -I./third/hiredis

OBJECT = main.o log.o util.o driver_loader.o config_loader.o driver_manager.o sqlbus.o redisop.o

LNKLIBS = -ldl
LNKLIBS += -L./library -lhiredis -lcjson

ifeq ($(ver), debug)
CFLAGS += $(CFLAG) -g3 -DDBUG_ON
LNKLIBS += -L./library -ldbug
else
CFLAGS += $(CFLAG)
endif

all:sqlbus

sqlbus:$(OBJECT)
	$(QUIET_LINK) $(OBJECT) $(CFLAGS) $(LNKLIBS) -o $@ $(THIRD_INC)

.c.o:
	$(QUIET_PROC) $(CFLAGS) -c $< $(THIRD_INC)

.PHONY:clean
clean:
	$(QUIET_REMOVE) -f $(OBJECT) sqlbus app.log
