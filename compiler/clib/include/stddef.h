#ifndef _STDDEF_H_
#define _STDDEF_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file stddef.h
    Lang: english
*/
#include <sys/_types.h>

/* This construction prevents these types from being multiply defined. */

/* This is CPU dependant */
#ifndef __AROS_PTRDIFF_T_DECLARED
#define __AROS_PTRDIFF_T_DECLARED
typedef __ptrdiff_t         ptrdiff_t;
#endif

#ifndef __AROS_SIZE_T_DECLARED
#define __AROS_SIZE_T_DECLARED
typedef __size_t            size_t;
#endif

#ifndef __AROS_WCHAR_T_DECLARED
#define __AROS_WCHAR_T_DECLARED
typedef __wchar_t           wchar_t;
#endif

#ifndef NULL
#define NULL		0
#endif

#ifndef offsetof
#define offsetof(type,member)		__offsetof(type,member)
#endif

#endif /* _STDDEF_H_ */
