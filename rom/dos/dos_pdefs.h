#ifndef _DOS_PDEFS_H
#define _DOS_PDEFS_H
/*
    Copyright (C) 1997-1998 AROS - The Amiga Research OS
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

/*
    Defines
*/

#define DosGetString(stringNum) \
    AROS_LC1(STRPTR, DosGetString, \
    AROS_LCA(LONG, stringNum, D0), \
    struct DosLibrary *, DOSBase, 163, Dos)

#endif /* _DOS_PDEFS_H */
