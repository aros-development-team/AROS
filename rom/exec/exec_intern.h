/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private data belonging to exec.library
    Lang:
*/
#ifndef __EXEC_INTERN_H__
#define __EXEC_INTERN_H__

/* This is a short file that contains a few things every Exec function needs */

#include <aros/debug.h> /* Needed for aros_print_not_implemented macro */
#include <aros/system.h>
#include <aros/libcall.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#include <exec_platform.h>

#define ALERT_BUFFER_SIZE 2048

/* Internals of this structure are host-specific, we don't know them here */
struct HostInterface;

/* A private portion of ExecBase */
struct IntExecBase
{
    struct ExecBase pub;
    struct List ResetHandlers;                  /* Reset handlers list                                   */
    struct Interrupt ColdResetHandler;          /* Reset handler that causes cold reboot */
    struct Interrupt WarmResetHandler;          /* Reset handler that causes warm reboot */
    struct MinList AllocMemList;                /* Mungwall allocations list                             */
    struct SignalSemaphore MemListSem;          /* Memory list protection semaphore                      */
    struct SignalSemaphore LowMemSem;           /* Lock for single-threading low memory handlers         */
    APTR   KernelBase;                          /* kernel.resource base                                  */
    struct Library *DebugBase;                  /* debug.library base                                    */
    ULONG  PageSize;                            /* Memory page size                                      */
    ULONG  IntFlags;                            /* Internal flags, see below                             */
    struct MsgPort *ServicePort;                /* Message port for service task                         */
    struct MinList TaskStorageSlots;            /* List of free slots, always one element with next slot */
    struct Exec_PlatformData PlatformData;      /* Platform-specific stuff                               */
    struct Task *SupervisorAlertTask;           /* Task that tries to display supervisor-level alerts    */
    IPTR SupervisorAlertTaskParams[2];
    char   AlertBuffer[ALERT_BUFFER_SIZE];      /* Buffer for alert text                                 */
};

#define PrivExecBase(base) ((struct IntExecBase *)base)
#define PD(base)   PrivExecBase(base)->PlatformData
#define KernelBase PrivExecBase(SysBase)->KernelBase
#define DebugBase  PrivExecBase(SysBase)->DebugBase

/* IntFlags */
#define EXECF_MungWall   0x0001 /* This flag can't be changed at runtime */
#define EXECF_StackSnoop 0x0002

/* Additional private task states */
#define TS_SERVICE 128

/* Puddle size, in slots. Must be at least 1 */
#define TASKSTORAGEPUDDLE 16

#if UseLVOs
extern void __AROS_InitExecBase (void);
#endif

struct ExecBase *PrepareExecBase(struct MemHeader *mh, struct TagItem *tags);
void InitExecBase(struct ExecBase *SysBase, ULONG negsize, struct TagItem *msg);
struct ExecBase *PrepareExecBaseMove(struct ExecBase *oldSysBase);
BOOL Exec_PreparePlatform(struct Exec_PlatformData *pdata, struct TagItem *tags);

void InitKickTags(struct ExecBase *SysBase);
UWORD GetSysBaseChkSum(struct ExecBase *sysbase);
void SetSysBaseChkSum(void);
BOOL IsSysBaseValid(struct ExecBase *sysbase);

IPTR cpu_SuperState();

void ServiceTask(struct ExecBase *SysBase);

#endif /* __EXEC_INTERN_H__ */
