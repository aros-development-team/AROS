#ifndef _DOS_PRIVATE_H
#define _DOS_PRIVATE_H

/* 
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Private function definitions for DOS.
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif
#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif

/*
    Prototypes
*/

AROS_LP1(STRPTR, DosGetString,
    AROS_LPA(LONG, stringNum, D0),
    struct DosLibrary *, DOSBase, 163, Dos)

#endif /* _DOS_PRIVATE_H */
