#ifndef _INTUITION_PDEFS_H
#define _INTUITION_PDEFS_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
    $Id

    Desc: Private function definitions for Intuition
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Defines
*/

#define LateIntuiInit(data) \
    AROS_LC1(BOOL, LateIntuiInit, \
    AROS_LCA(APTR, data, A0), \
    struct IntuitionBase *, IntuitionBase, 120, Intuition)

#endif /* _INTUITION_PDEFS_H */
