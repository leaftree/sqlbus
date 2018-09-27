
/**
 * debug.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-09-02 15:37:00
 * Last Modified : 2018-09-02 15:37:00
 */

#ifndef __DEBUG_H__
#define __DEBUG_H__

__BEGIN_DECLS

void sig_segfault_handler(int sig, siginfo_t *info, void *secret);

__END_DECLS

#endif /* __DEBUG_H__ */
