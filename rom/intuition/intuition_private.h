#ifndef _INTUITION_PRIVATE_H
#define _INTUITION_PRIVATE_H
/* 
    Copyright (C) 1998 AROS - The Amiga Research OS
    $Id$

    Desc: Private function definitions for Intuition
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
#   include "intuition_pinlines.h"
#else
#   include "intuition_pdefs.h"
#endif

/*
    Prototypes
*/

AROS_LP1(BOOL, LateIntuiInit,
    AROS_LPA(APTR, data, A0),
    struct IntuitionBase *, IntuitionBase, 120, Intuition)

#endif /* _INTUITION_PRIVATE_H */
