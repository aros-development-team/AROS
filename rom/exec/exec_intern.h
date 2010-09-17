/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
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

#include <exec_platform.h>

/* Internals of this structure are host-specific, we don't know them here */
struct HostInterface;

/* A private portion of ExecBase */
struct IntExecBase
{
    struct ExecBase pub;
    struct List ResetHandlers;			/* Reset handlers list       */
    ULONG  IntFlags;				/* Internal flags, see below */
    APTR   KernelBase;				/* kernel.resource base      */
    struct Exec_PlatformData PlatformData;	/* Platform-specific stuff   */
};

#define PrivExecBase(base) ((struct IntExecBase *)base)
#define PD(base)   PrivExecBase(base)->PlatformData
#define KernelBase PrivExecBase(SysBase)->KernelBase

/* IntFlags */
#define EXECF_MungWall 0x0001

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

struct ExecBase *PrepareExecBase(struct MemHeader *mh, char *args, struct HostInterface *hif);
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pdata, struct HostInterface *hif);

#endif /* __EXEC_INTERN_H__ */
