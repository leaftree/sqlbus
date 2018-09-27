
/**
 * setproctitle.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-02 12:57:44
 * Last Modified : 2018-04-02 12:57:44
 */

#ifndef __SETPROCTITLE_H__
#define __SETPROCTITLE_H__

__BEGIN_DECLS

void  set_process_name(char *name);
char *get_process_name(char *name);

void set_process_title_init(char* argv[]);
void set_process_title(char* argv[], char* title);

__END_DECLS

#endif /* __SETPROCTITLE_H__ */
