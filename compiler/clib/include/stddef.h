#ifndef _STDDEF_H_
#define _STDDEF_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ANSI-C header file stddef.h
    Lang: english
*/

#include <aros/systypes.h>

/* This construction prevents these types from being multiply defined. */

/* This is CPU dependant */
#ifdef	_AROS_PTRDIFF_T_
typedef _AROS_PTRDIFF_T_    ptrdiff_t;
#undef	_AROS_PTRDIFF_T_
#endif

#ifdef  _AROS_SIZE_T_
typedef	_AROS_SIZE_T_	    size_t;
#undef	_AROS_SIZE_T_
#endif

#ifdef	_AROS_WCHAR_T_
typedef _AROS_WCHAR_T_	    wchar_t;
#undef	_AROS_WCHAR_T_
#endif

#ifndef NULL
#define NULL		0
#endif

#ifndef offsetof
#define offsetof(type,member)		__offsetof(type,member)
#endif

#endif /* _STDDEF_H_ */
