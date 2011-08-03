/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

#define ALERT_BUFFER_SIZE 2048

/* Internals of this structure are host-specific, we don't know them here */
struct HostInterface;

/* A private portion of ExecBase */
struct IntExecBase
{
    struct ExecBase pub;
    struct List ResetHandlers;			/* Reset handlers list      				*/
    struct MinList AllocMemList;		/* Mungwall allocations list				*/
    struct SignalSemaphore MemListSem;		/* Memory list protection semaphore			*/
    struct SignalSemaphore LowMemSem;		/* Lock for single-threading low memory handlers	*/
    APTR   KernelBase;				/* kernel.resource base      				*/
    struct Library *DebugBase;			/* debug.library base					*/
    ULONG  PageSize;				/* Memory page size	     				*/
    ULONG  IntFlags;				/* Internal flags, see below 				*/
    struct MsgPort *RemTaskPort;                /* port used for RemTask() memory cleanup               */
    struct Exec_PlatformData PlatformData;	/* Platform-specific stuff   				*/
    char   AlertBuffer[ALERT_BUFFER_SIZE];	/* Buffer for alert text     				*/
};

#define PrivExecBase(base) ((struct IntExecBase *)base)
#define PD(base)   PrivExecBase(base)->PlatformData
#define KernelBase PrivExecBase(SysBase)->KernelBase
#define DebugBase  PrivExecBase(SysBase)->DebugBase

/* IntFlags */
#define EXECF_MungWall   0x0001	/* This flag can't be changed at runtime */
#define EXECF_StackSnoop 0x0002

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

APTR allocBootMem(struct MemHeader *mh, ULONG size);
struct ExecBase *PrepareExecBase(struct MemHeader *mh, struct TagItem *tags);
struct ExecBase *PrepareExecBaseMove(struct ExecBase *oldSysBase);
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pdata, struct TagItem *tags);

void InitKickTags(struct ExecBase *SysBase);
UWORD GetSysBaseChkSum(struct ExecBase *sysbase);
void SetSysBaseChkSum(void);
BOOL IsSysBaseValid(struct ExecBase *sysbase);

IPTR cpu_SuperState();

#endif /* __EXEC_INTERN_H__ */
