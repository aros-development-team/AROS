/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REXXSYSLIB_INTERN_H
#define REXXSYSLIB_INTERN_H

/* Include files */

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif
#ifndef PROTO_REXXSYSLIB_H
#   include <proto/rexxsyslib.h>
#endif
#ifndef EXEC_MEMORY_H
#   include <exec/memory.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef AROS_DEBUG_H
#   include <aros/debug.h>
#endif
#ifndef REXX_STORAGE_H
#   include <rexx/storage.h>
#endif
#ifndef REXX_RXSLIB_H
#   include <rexx/rxslib.h>
#endif
#include <stdlib.h>


/* Some external stuff (rexxsyslib_init.c) */


struct RexxSysBase_intern; /* prereference */

/************************/
/* Internal structures	*/
/************************/

struct RexxSysBase_intern
{
    struct RxsLib          library;
    char *                 rexxmsgid;
    struct SignalSemaphore semaphore;
};

#define RSBI(base) ((struct RexxSysBase_intern *)base)

#endif /* REXXSYSLIB_INTERN_H */
