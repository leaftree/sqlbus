
#================================================================
#   Copyright (C) 2018 Liu YunFeng. All rights reserved.
#   
#   文件名称：Makefile
#   创 建 者：Liu YunFeng
#   创建日期：2018年03月10日
#   描    述：
#
#================================================================

CCCOLOR   = "\033[1;33m"
LINKCOLOR = "\033[34;1m"
SRCCOLOR  = "\033[31m"
RMCOLOR   = "\033[1;31m"
CPCOLOR   = "\033[1;31m"
BINCOLOR  = "\033[37;1m"
MAKECOLOR = "\033[32;1m"
ENDCOLOR  = "\033[0m"
CHGCOLOR  = "\033[1;33m"

QUIET_CC  = @printf '%b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_CP  = @printf '%b %b\n' $(CPCOLOR)COPY$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_RM  = @printf '%b %b\n' $(RMCOLOR)REMOVE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_EXE = @printf '%b %b\n' $(BINCOLOR)EXEC$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LNK = @printf '%b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_CHG = @printf '%b %b\n' $(LINKCOLOR)CHANGE$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;

QUIET_PROC=$(QUIET_CC)$(CC)
QUIET_EXEC=$(QUIET_EXE)exec
QUIET_COPY=$(QUIET_CP)cp -f
QUIET_REMOVE=$(QUIET_RM)rm -f
QUIET_LINK=$(QUIET_LNK)gcc -gstabs+3 
QUIET_CHDIR=$(QUIET_CHG)cd
DIALOG=echo -e

WORKDIR=$(PWD)
STATIC_LIB=$(WORKDIR)/lib

default:all

all: cjson hiredis sqlbus

cjson:
	@$(DIALOG) $(CCCOLOR)BUILDING $@ $(ENDCOLOR)
	$(QUIET_CHDIR) third/cJSON && $(MAKE) static
	$(QUIET_COPY) third/cJSON/libcjson.a $(STATIC_LIB)
	@$(DIALOG) $(RMCOLOR)CLEANUP $@ $(ENDCOLOR)
	$(QUIET_CHDIR) third/cJSON && $(MAKE) clean

hiredis:
	@$(DIALOG) $(CCCOLOR)BUILDING $@ $(ENDCOLOR)
	$(QUIET_CHDIR) third/hiredis && $(MAKE) static
	$(QUIET_COPY) third/hiredis/libhiredis.a $(STATIC_LIB)
	@$(DIALOG) $(RMCOLOR)CLEANUP $@ $(ENDCOLOR)
	$(QUIET_CHDIR) third/hiredis && $(MAKE) clean

sqlbus:
	$(QUIET_CHDIR) src && $(MAKE) all

.PHONY:clean
clean:
	$(QUIET_REMOVE) app.log
	$(QUIET_REMOVE) sqlbus
	$(QUIET_CHDIR) src && $(MAKE) clean
	$(QUIET_CHDIR) third/cJSON && $(MAKE) clean
	$(QUIET_CHDIR) third/hiredis && $(MAKE) clean
