#ifndef _DOS_PRIVATE_H
#define _DOS_PRIVATE_H
/* 
    Copyright (C) 1998 AROS - The Amiga Replacement OS
    $Id$

    Desc: Private function definitions for DOS
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

#if defined(_AMIGA) && defined(__GNUC__)
#   ifndef NO_INLINE_STDARG
#	define NO_INLINE_STDARG
#   endif
#   include "dos_pinlines.h"
#else
#   include "dos_pdefs.h"
#endif

/*
    Prototypes
*/

AROS_LP1(STRPTR, DosGetString,
    AROS_LPA(ULONG, stringNum, D0),
    struct DosLibrary *, DOSBase, 163, Dos)

#endif /* _DOS_PRIVATE_H */
