
/**
 * util.h
 *
 * Copyright (C) 2018 by Liu YunFeng.
 *
 *        Create : 2018-03-07 13:18:17
 * Last Modified : 2018-03-07 13:18:17
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdlib.h>

#ifndef __PAGE_SIZE
# define __PAGE_SIZE (sysconf(_SC_PAGE_SIZE))
#endif
#ifndef aligned
# define aligned(size, align) (roundup(size, align))
#endif

# define pagesize_aligned(size) aligned(size, __PAGE_SIZE)

__BEGIN_DECLS

void rtrim(char *str, int len);
void ltrim(char *str, int len);

__END_DECLS

#endif /* __UTIL_H__ */
