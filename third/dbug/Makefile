
CC = gcc
CFLAGS = -pg -O2 -Wshadow -fPIC

#INC=$(HOME)/appl/inc
INC=.
#PREFIX := $(HOME)/appl
PREFIX := .
INSTALL := cp -a
INCLUDE_PATH := $(PREFIX)/inc

#BINPRJDIR
#BINRARY_PATH := $(RUNDIR)/bin
BINRARY_PATH := .

OBJS := dbug.o

PBLIB=libdbug.so

all: $(PBLIB) 

$(PBLIB):$(OBJS)
	$(CC) $(CFLAGS) -shared -fPIC -o $(BINRARY_PATH)/$@ $^
	-@$(RM) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -I$(INCLUDE_PATH) -c $<

.PHONY:clean
clean:
	-@rm -f $(OBJS)
	-@cd $(BINRARY_PATH) && rm -f $(PBLIB)
