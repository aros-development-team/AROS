/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/12/09 10:54:49  aros
    Added (almost empty) versions for all missing functions. If they are called,
    they just print "Functions %s not implemented" and return an error if
    possible.

    Revision 1.2  1996/08/01 17:41:27  digulla
    Added standard header for all files

    Desc:
    Lang:
*/
#ifndef __EXEC_INTERN_H__
#define __EXEC_INTERN_H__

/* This is a short file that contains a few things every Exec function
    needs */

#ifndef AROS_OPTIONS_H
#   include <aros/options.h>
#endif
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef CLIB_EXEC_PROTOS_H
#   include <clib/exec_protos.h>
#endif

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

void aros_print_not_implemented (char * name);

#endif /* __EXEC_INTERN_H__ */
