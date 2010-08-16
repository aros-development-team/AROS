/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private data belonging to exec.library
    Lang:
*/
#ifndef __EXEC_INTERN_H__
#define __EXEC_INTERN_H__

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

/* This is a short file that contains a few things every Exec function
    needs */

#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif
#ifndef AROS_LIBCALL_H
#   include <aros/libcall.h>
#endif
#ifndef EXEC_EXECBASE_H
#   include <exec/execbase.h>
#endif
#ifndef PROTO_EXEC_H
#   include <proto/exec.h>
#endif

/* A private portion of ExecBase */
struct IntExecBase
{
    struct ExecBase pub;
    struct List ResetHandlers;	/* Reset handlers list	     */
    ULONG  IntFlags;		/* Internal flags, see below */
};

#define PrivExecBase(base) ((struct IntExecBase *)base)

/* IntFlags */
#define EXECF_MungWall 0x0001

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif


/* These are the bit definitions of the SysFlags and AttnResched flags.
    They are listed here more as somewhere to list them.
*/

#define SFB_SoftInt         5   /* There is a software interrupt */
#define SFF_SoftInt         (1L<<5)

#define ARB_AttnSwitch      7   /* Delayed Switch() pending */
#define ARF_AttnSwitch      (1L<<7)
#define ARB_AttnDispatch   15   /* Delayed Dispatch() pending */
#define ARF_AttnDispatch    (1L<<15)

ULONG **AROS_SLIB_ENTRY(RomTagScanner,Exec)(struct ExecBase *, UWORD *ranges[]);
struct ExecBase *PrepareExecBase(struct MemHeader *mh, char *args);

#endif /* __EXEC_INTERN_H__ */
