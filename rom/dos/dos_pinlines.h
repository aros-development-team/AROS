#ifndef _DOS_PINLINES_H
#define _DOS_PINLINES_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Private inlines for dos.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef DOS_BASE_NAME
#define DOS_BASE_NAME DOSBase
#endif

#define DosGetString(stringnum) \
    LP1( , STRPTR , DosGetString, \
	ULONG, (stringnum), d0, \
	DOS_BASE_NAME )

#endif _DOS_PINLINES_H
