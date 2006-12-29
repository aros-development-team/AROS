/*
    Copyright � 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef REXXSUPPORT_INTERN_H
#define REXXSUPPORT_INTERN_H

/* Include files */

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
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
#include <rexx/rxslib.h>

/* Some external stuff (rexxsupport_init.c) */
struct RexxSupportBase_intern; /* prereference */

/************************/
/* Internal structures	*/
/************************/

struct RexxSupportBase_intern
{
    struct Library lib;
    struct List openports;
};

#define RSBI(base) ((struct RexxSupportBase_intern *)base)

/***********************************/
/* Definittion for ARexx functions */
/***********************************/

#define NEST_VAR "__RXSUPP__.__NESTING__"

#endif /* REXXSUPPORT_INTERN_H */
