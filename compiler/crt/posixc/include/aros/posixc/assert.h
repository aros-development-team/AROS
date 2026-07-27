/*
    Copyright (C) 2025-2026, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Posix header file assert.h
    Lang: english
*/

/* assert (and assert_perror) are redefined on every inclusion,
   according to the current state of NDEBUG - deliberately outside
   the include guard. */
#include <aros/stdc/assert.h>

#ifdef assert_perror
#undef assert_perror
#endif

#ifndef NDEBUG
#define assert_perror(errnum) \
    ((errnum) == 0 ? (void)0 : __assert_perror(errnum, __FILE__, __LINE__))
#else
#define assert_perror(errnum) ((void)0)
#endif

#ifndef _POSIXC_ASSERT_H_
#define _POSIXC_ASSERT_H_

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
void __assert_perror(int errnum, const char *file, unsigned int line);
#ifdef __cplusplus
}
#endif

#endif /* _POSIXC_ASSERT_H_ */
