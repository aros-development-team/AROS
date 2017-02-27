/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    Desc:

        Exposes a Public resource on smp systems that allows user space code
        to create (spin) locks, and exposes locking on exec Lists.

        e.g. LDDemon uses this to lock access to the system lists
        when scanning devices/libraries etc - since are not allowed
        to directly call the kernel locking functions or access the exec locks.

    Lang: english
*/

#include <aros/config.h>

#if defined(__AROSEXEC_SMP__)

#define DEBUG 0

#include <aros/debug.h>
#include <proto/exec.h>

#include "exec_debug.h"
#include "exec_intern.h"
#include "exec_locks.h"

int ExecLock__InternObtainSystemLock(struct List *systemList, ULONG mode, ULONG flags)
{
    spinlock_t *sysListLock = NULL;
    D(char *name = "??");

    if (systemList)
    {
        if (&SysBase->ResourceList == systemList) {
            D(name="ResourceList");
            sysListLock = &PrivExecBase(SysBase)->ResourceListSpinLock;
        }
        else if (&SysBase->DeviceList == systemList) {
            D(name="DeviceList");
            sysListLock = &PrivExecBase(SysBase)->DeviceListSpinLock;
        }
        else if (&SysBase->IntrList == systemList) {
            D(name="IntrList");
            sysListLock = &PrivExecBase(SysBase)->IntrListSpinLock;
        }
        else if (&SysBase->LibList == systemList) {
            D(name="LibList");
            sysListLock = &PrivExecBase(SysBase)->LibListSpinLock;
        }
        else if (&SysBase->PortList == systemList) {
            D(name="PortList");
            sysListLock = &PrivExecBase(SysBase)->PortListSpinLock;
        }
        else if (&SysBase->SemaphoreList == systemList) {
            D(name="SemaphoreList");
            sysListLock = &PrivExecBase(SysBase)->SemListSpinLock;
        }
        else if (&SysBase->MemList == systemList) {
            D(name="MemList");
            sysListLock = &PrivExecBase(SysBase)->MemListSpinLock;
        }
        else if (&SysBase->TaskReady == systemList) {
            D(name="TaskReady");
            sysListLock = &PrivExecBase(SysBase)->TaskReadySpinLock;
        }
        else if (&SysBase->TaskWait == systemList) {
            D(name="TaskWait");
            sysListLock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
        }
        else if (&PrivExecBase(SysBase)->TaskRunning == systemList) {
            D(name="TaskRunning");
            sysListLock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
        }
        else if (&PrivExecBase(SysBase)->TaskSpinning == systemList) {
            D(name="TaskSpinning");
            sysListLock = &PrivExecBase(SysBase)->TaskSpinningLock;
        }
    }
    D(bug("[Exec:Lock] %s(), List='%s' (%p), mode=%s, flags=%d\n", __func__, name, systemList, mode == SPINLOCK_MODE_WRITE ? "write":"read", flags));
    if (sysListLock)
    {
        if (flags && LOCKF_DISABLE)
            Disable();
        if (flags && LOCKF_FORBID)
            Forbid();

        EXEC_SPINLOCK_LOCK(sysListLock, NULL, mode);

        return TRUE;
    }
    return FALSE;
}

void ExecLock__InternReleaseSystemLock(struct List *systemList, ULONG flags)
{
    spinlock_t *sysListLock = NULL;
    D(char *name = "??");

    if (systemList)
    {
        if (&SysBase->ResourceList == systemList) {
            D(name="ResourceList");
            sysListLock = &PrivExecBase(SysBase)->ResourceListSpinLock;
        }
        else if (&SysBase->DeviceList == systemList) {
            D(name="DeviceList");
            sysListLock = &PrivExecBase(SysBase)->DeviceListSpinLock;
        }
        else if (&SysBase->IntrList == systemList) {
            D(name="IntrList");
            sysListLock = &PrivExecBase(SysBase)->IntrListSpinLock;
        }
        else if (&SysBase->LibList == systemList) {
            D(name="LibList");
            sysListLock = &PrivExecBase(SysBase)->LibListSpinLock;
        }
        else if (&SysBase->PortList == systemList) {
            D(name="PortList");
            sysListLock = &PrivExecBase(SysBase)->PortListSpinLock;
        }
        else if (&SysBase->SemaphoreList == systemList) {
            D(name="SemaphoreList");
            sysListLock = &PrivExecBase(SysBase)->SemListSpinLock;
        }
        else if (&SysBase->MemList == systemList) {
            D(name="MemList");
            sysListLock = &PrivExecBase(SysBase)->MemListSpinLock;
        }
        else if (&SysBase->TaskReady == systemList) {
            D(name="TaskReady");
            sysListLock = &PrivExecBase(SysBase)->TaskReadySpinLock;
        }
        else if (&SysBase->TaskWait == systemList) {
            D(name="TaskWait");
            sysListLock = &PrivExecBase(SysBase)->TaskWaitSpinLock;
        }
        else if (&PrivExecBase(SysBase)->TaskRunning == systemList) {
            D(name="TaskRunning");
            sysListLock = &PrivExecBase(SysBase)->TaskRunningSpinLock;
        }
        else if (&PrivExecBase(SysBase)->TaskSpinning == systemList) {
            D(name="TaskSpinning");
            sysListLock = &PrivExecBase(SysBase)->TaskSpinningLock;
        }
    }
    D(bug("[Exec:Lock] %s(), list='%s' (%p), flags=%d\n", __func__, name, systemList, flags));
    if (sysListLock)
    {
        EXEC_SPINLOCK_UNLOCK(sysListLock);

        if (flags & LOCKF_FORBID)
            Permit();
        if (flags & LOCKF_DISABLE)
            Enable();
    }
}
#if 0
/* Locking mechanism for userspace */
void *ExecLock__AllocLock()
{
    spinlock_t *publicLock;

    D(bug("[Exec:Lock] %s()\n", __func__));

    if ((publicLock = (spinlock_t *)AllocMem(sizeof(spinlock_t), MEMF_ANY)) != NULL)
    {
        EXEC_SPINLOCK_INIT(publicLock);
    }
    return publicLock;
}

void *ExecLock__ObtainLock(void *lock, ULONG mode)
{
    D(bug("[Exec:Lock] %s()\n", __func__));

    if (lock)
    {
        EXEC_SPINLOCK_LOCK(lock, NULL, mode);
        return (void *)TRUE;
    }
    return NULL;
}

void ExecLock__ReleaseLock(void *lock)
{
    D(bug("[Exec:Lock] %s()\n", __func__));

    if (lock)
    {
        EXEC_SPINLOCK_UNLOCK(lock);
    }
}

void ExecLock__FreeLock(void *lock)
{
    D(bug("[Exec:Lock] %s()\n", __func__));

    if (lock)
    {
        FreeMem(lock, sizeof(spinlock_t));
    }
}
#endif

AROS_LH3 (int, ObtainSystemLock,
    AROS_LHA(struct List *, systemList, A0),
    AROS_LHA(ULONG, mode, D0),
    AROS_LHA(ULONG, flags, D1),
    struct ExecLockBase *, ExecLockBase, 1, ExecLock
)
{
    AROS_LIBFUNC_INIT

    D(bug("[Exec:Lock] %s()\n", __func__));

    return ExecLock__InternObtainSystemLock(systemList, mode, flags);

    AROS_LIBFUNC_EXIT
}

AROS_LH2 (void, ReleaseSystemLock,
    AROS_LHA(struct List *, systemList, A0),
    AROS_LHA(ULONG, flags, D1),
    struct ExecLockBase *, ExecLockBase, 2, ExecLock
)
{
    AROS_LIBFUNC_INIT

    D(bug("[Exec:Lock] %s()\n", __func__));

    ExecLock__InternReleaseSystemLock(systemList, flags);

    AROS_LIBFUNC_EXIT
}

const APTR ExecLock__FuncTable[]=
{
    &AROS_SLIB_ENTRY(ObtainSystemLock,ExecLock,1),
    &AROS_SLIB_ENTRY(ReleaseSystemLock,ExecLock,2),
    (void *)-1
};


APTR ExecLock__PrepareBase(struct MemHeader *mh)
{
    APTR ExecLockResBase;
    struct ExecLockBase *ExecLockBase;

    D(bug("[Exec:Lock] %s()\n", __func__));

    ExecLockResBase = Allocate(mh, sizeof(struct ExecLockBase) + sizeof(ExecLock__FuncTable));
    ExecLockBase = (struct ExecLockBase *)((IPTR)ExecLockResBase + sizeof(ExecLock__FuncTable));

    MakeFunctions(ExecLockBase, ExecLock__FuncTable, NULL);

    ExecLockBase->el_Node.ln_Name = "execlock.resource";
    ExecLockBase->el_Node.ln_Type = NT_RESOURCE;
    ExecLockBase->ObtainSystemLock = ExecLock__InternObtainSystemLock;
    ExecLockBase->ReleaseSystemLock = ExecLock__InternReleaseSystemLock;

    AddResource(ExecLockBase);

    return ExecLockBase;
}

#endif
