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

struct ExecLockBase
{
    struct Library el_Lib;
};

void *ExecLock__ObtainSystemLock(struct List *systemList, ULONG mode)
{
    spinlock_t *sysListLock = NULL;

    D(bug("[Exec:Lock] %s()\n", __func__));

    if (systemList)
    {
        if (&SysBase->ResourceList == systemList)
            sysListLock = &PrivExecBase(SysBase)->ResourceListSpinLock;
        else if (&SysBase->DeviceList == systemList)
            sysListLock = &PrivExecBase(SysBase)->DeviceListSpinLock;
        else if (&SysBase->IntrList == systemList)
            sysListLock = &PrivExecBase(SysBase)->IntrListSpinLock;
        else if (&SysBase->LibList == systemList)
            sysListLock = &PrivExecBase(SysBase)->LibListSpinLock;
        else if (&SysBase->PortList == systemList)
            sysListLock = &PrivExecBase(SysBase)->PortListSpinLock;
        else if (&SysBase->SemaphoreList == systemList)
            sysListLock = &PrivExecBase(SysBase)->SemListSpinLock;
    }
    if (sysListLock)
    {
        EXEC_SPINLOCK_LOCK(sysListLock, NULL, mode);
        return (void *)TRUE;
    }
    return NULL;
}

void ExecLock__ReleaseSystemLock(struct List *systemList)
{
    spinlock_t *sysListLock = NULL;

    D(bug("[Exec:Lock] %s()\n", __func__));

    if (systemList)
    {
        if (&SysBase->ResourceList == systemList)
            sysListLock = &PrivExecBase(SysBase)->ResourceListSpinLock;
        else if (&SysBase->DeviceList == systemList)
            sysListLock = &PrivExecBase(SysBase)->DeviceListSpinLock;
        else if (&SysBase->IntrList == systemList)
            sysListLock = &PrivExecBase(SysBase)->IntrListSpinLock;
        else if (&SysBase->LibList == systemList)
            sysListLock = &PrivExecBase(SysBase)->LibListSpinLock;
        else if (&SysBase->PortList == systemList)
            sysListLock = &PrivExecBase(SysBase)->PortListSpinLock;
        else if (&SysBase->SemaphoreList == systemList)
            sysListLock = &PrivExecBase(SysBase)->SemListSpinLock;
    }
    if (sysListLock)
    {
        EXEC_SPINLOCK_UNLOCK(sysListLock);
    }
}

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

struct Library *ExecLock__PrepareBase(struct MemHeader *mh)
{
    struct ExecLockBase *ExecLockBase;

    D(bug("[Exec:Lock] %s()\n", __func__));

    ExecLockBase = Allocate(mh, sizeof(struct ExecLockBase));

    // TODO - Initialise the resource and add to the system.

    return (struct Library *)ExecLockBase;
}

#endif
