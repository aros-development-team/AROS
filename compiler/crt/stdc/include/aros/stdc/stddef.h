#ifndef _STDC_STDDEF_H_
#define _STDC_STDDEF_H_

/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: C99 header file stddef.h
*/

#include <aros/types/ptrdiff_t.h>
#include <aros/types/size_t.h>
#include <aros/types/wchar_t.h>
#include <aros/types/null.h>

#ifdef __GNUC__
#define offsetof(type, member)      __builtin_offsetof(type, member)
#else
#define offsetof(type, member)	    ((size_t)(&((type *)0)->member))
#endif

#endif /* _STDC_STDDEF_H_ */
