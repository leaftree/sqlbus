CC = gcc -Wall

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

PROC     = proc
ORAINC   = $(ORACLE_HOME)/precomp/public
EXTINC   = $(ORACLE_HOME)/rdbms/public
PROCINC  = INCLUDE=$(ORAINC) INCLUDE=$(EXTINC)
PROCFLAG = sqlcheck=full def_sqlcode=yes code=ansi_c lines=yes

ORALIBS = -L$(ORACLE_HOME)/lib -lclntsh -lsqlplus
LNKLIBS = -lm $(ORALIBS)

ifeq ($(ver), debug)
CFLAGS += -g3 -DDBUG_ON
LNKLIBS += -ldbug
endif

src = ora.c util.c server.c
serverObj = util.o server.o

LIBSO = libora.so
PACKAGE = server
TARGET = $(PACKAGE) $(LIBSO)

all:$(TARGET)

$(PACKAGE):$(serverObj) $(LIBSO)
	$(QUIET_PROC) $(CFLAGS) $(serverObj) -o $@ -L. -lora $(LNKLIBS)

$(LIBSO): ora.o
	$(QUIET_PROC) $(CFLAGS) $< util.o -shared -fPIC -o $@ $(LNKLIBS)

ora.c: ora.pc
	$(PROC) $(PROCINC) $(PROCFLAG) INAME=ora.pc ONAME=ora.c

.c.o:
	$(QUIET_PROC) $(CFLAGS) $(INC) -I$(ORAINC) -I$(EXTINC) -c $*.c -shared -fPIC

tags:$(src)
	ctags *.c *.h

.PHONY:clean
clean:
	$(QUIET_REMOVE) -f $(TARGET)
	$(QUIET_REMOVE) -f core.*
	$(QUIET_REMOVE) -f *.o
	$(QUIET_REMOVE) -f tags
	$(QUIET_REMOVE) -f ora.lis ora.c
