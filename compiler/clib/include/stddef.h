#ifndef _STDDEF_H
#define _STDDEF_H

/*
    (C) 1995-96 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI-C header file stddef.h
    Lang: english
*/
#ifndef _SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _WCHAR_T
#define _WCHAR_T
typedef unsigned long wchar_t;
#endif
 
#ifndef offsetof
#define offsetof(TYPE,MEMBER)     ((size_t) &((TYPE *)0)->MEMBER)
#endif

#endif /* _STDDEF_H */
