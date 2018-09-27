
/**
 * xmalloc.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-04-15 01:53:05
 * Last Modified : 2018-04-15 01:53:05
 */

#ifndef __XMALLOC_H__
#define __XMALLOC_H__

#include <malloc.h>
#include <string.h>

#ifdef xFree
# undef xFree
#endif

#ifdef xMalloc
# undef xMalloc
#endif

#ifdef xStrdup
# undef xStrdup
#endif

#ifndef MEM_DEBUG_ON
# define xStrdup(str) strdup(str)
# define xMalloc(size) malloc(size)
# define xFree(pmem) do {free(pmem);pmem=NULL;}while(0)
#else
# define xStrdup(str) _afc_strdup(str, __FILE__, __func__, __LINE__)
# define xMalloc(size) _afc_malloc(size, __FILE__, __func__, __LINE__)
# define xFree(pmem) do { _afc_free(pmem, __FILE__, __func__, __LINE__); pmem = NULL; }while(0)

__BEGIN_DECLS

void  _afc_free(void *pmem, const char *file, const char *func, int line);
void *_afc_malloc(int size, const char *file, const char *func, int line);
char *_afc_strdup(char *str, const char *file, const char *func, int line);

__END_DECLS

#endif /* MEM_DEBUG_ON */

#endif /* __XMALLOC_H__ */
