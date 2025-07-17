#ifndef _POSIXC_ASSERT_H_
#define _POSIXC_ASSERT_H_

/*
    Copyright © 2025, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Posix header file assert.h
    Lang: english
*/

#include <aros/stdc/assert.h>

#ifndef NDEBUG
#include <errno.h>
#define assert_perror(errnum) \
    ((errnum) == 0 ? (void)0 : __assert_perror(errnum, __FILE__, __LINE__))

#ifdef __cplusplus
extern "C" {
#endif
void __assert_perror(int errnum, const char *file, unsigned int line);
#ifdef __cplusplus
}
#endif

#else
#define assert_perror(errnum) ((void)0)
#endif

#endif /* _POSIXC_ASSERT_H_ */
