#ifndef _SYS__TYPES_H_
#define _SYS__TYPES_H_
/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/cpu.h>

/*
 * Standard type definitions:
 *  These are defined here prefixed by __ to allow us to use them as
 *  proper types, but so that they are not declared types. ISO C and POSIX
 *  both only allow certain types to be declared at certain times.
 */
typedef signed AROS_8BIT_TYPE           __int8_t;
typedef unsigned AROS_8BIT_TYPE         __uint8_t;
typedef signed AROS_16BIT_TYPE          __int16_t;
typedef unsigned AROS_16BIT_TYPE        __uint16_t;
typedef signed AROS_32BIT_TYPE          __int32_t;
typedef unsigned AROS_32BIT_TYPE        __uint32_t;
typedef signed AROS_64BIT_TYPE          __int64_t;
typedef unsigned AROS_64BIT_TYPE        __uint64_t;
typedef signed AROS_INTPTR_TYPE         __intptr_t;
typedef unsigned AROS_INTPTR_TYPE       __uintptr_t;

/*
 * Non-size specific types. Used by various different functions.
 * See the entries in <sys/types.h> for actual descriptions of what the
 * types might meen.
 */
typedef __int32_t               __blkcnt_t;
typedef __int32_t               __blksize_t;
typedef char *                  __caddr_t;          /* Not POSIX */
typedef __int32_t               __clockid_t;
typedef unsigned long           __clock_t;
typedef __int32_t               __dev_t;
typedef __int32_t               __fsblkcnt_t;
typedef __int32_t               __fsfilcnt_t;
typedef __uint32_t              __gid_t;
typedef __uint32_t              __id_t;
typedef __uint32_t              __ino_t;
typedef __int32_t               __key_t;
typedef __uint16_t              __mode_t;
typedef __uint16_t              __nlink_t;
typedef __int32_t               __off_t;            /* XXX Large Files? */
typedef __uintptr_t             __pid_t;
typedef long                    __ptrdiff_t;        /* XXX __intptr_t? */
typedef __uint8_t               __sa_family_t;
typedef unsigned int            __size_t;           /* XXX Large Files? */
typedef __uint32_t              __socklen_t;
typedef int                     __ssize_t;
typedef __int32_t               __suseconds_t;
typedef __uint32_t              __time_t;           /* XXX Limiting? */
typedef int                     __timer_t;
typedef __uint32_t              __uid_t;
typedef __uint32_t              __useconds_t;
typedef char *                  __va_list;

/*
 * __wchar_t is defined unsigned for compatibility with locale.library's
 * representation of wide characters. __wint_t needs to be signed so that
 * it can represent WEOF (-1).
 */
typedef __uint32_t              __wchar_t;
typedef __int32_t               __wint_t;

/*
 * HACK!
 * BSD derived systems take their locale handling from Plan9. This means
 * that they expect <sys/_types.h> to typedef __rune_t. Unfortunately since
 * we use the compilers <stddef.h> for BSD systems (for now), this means
 * it wants __rune_t to be valid.
 *
 * I tried to handle this in configure.in as a #define, but that just broke
 * something else.
 *
 * XXX This should be removed when we always use our own compiler.
 */
typedef int                     __rune_t;


/* Both <stddef.h> and <sys/types.h> define this type */
#define __offsetof(type,field)          ((size_t)(&((type *)0)->field))

#endif /* _SYS__TYPES_H_ */
