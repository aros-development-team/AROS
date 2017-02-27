#ifndef _EXEC_LOCKS_H
#define _EXEC_LOCKS_H

/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>
#include <exec/nodes.h>
#include <resources/execlock.h>
#include "exec_debug.h"
#include "exec_intern.h"

/*
 * NB : This file is only for internal Exec access to the ExecLock resource functions.
 *      All other code should use the public execlock.resource interface(s).
 */

struct ExecLockBase
{
    struct Node el_Node;

    int         (*ObtainSystemLock)(struct List *list, ULONG mode, ULONG flags);
    void        (*ReleaseSystemLock)(struct List *list, ULONG flags);
    void *      (*AllocLock)();
    void        (*FreeLock)(void *lock);
    int         (*ObtainLock)(void *lock, ULONG mode, ULONG flags);
    void        (*ReleaseLock)(void *lock, ULONG flags);
};

#if defined (__AROSEXEC_SMP__)

#define EXEC_LOCK_LIST_WRITE_AND_FORBID(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_WRITE, LOCKF_FORBID); \
    } else Forbid(); } while(0)

#define EXEC_LOCK_LIST_WRITE_AND_DISABLE(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_WRITE, LOCKF_DISABLE); \
    } else Disable(); } while(0)

#define EXEC_LOCK_LIST_READ_AND_FORBID(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_READ, LOCKF_FORBID); \
    } else Forbid(); } while(0)

#define EXEC_LOCK_LIST_READ_AND_DISABLE(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_READ, LOCKF_DISABLE); \
    } else Disable(); } while(0)

#define EXEC_UNLOCK_LIST_AND_PERMIT(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ReleaseSystemLock(list, LOCKF_FORBID); \
    } else Permit(); } while(0)

#define EXEC_UNLOCK_LIST_AND_ENABLE(list)     do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ReleaseSystemLock(list, LOCKF_DISABLE); \
    } else Enable(); } while(0)

#define EXEC_LOCK_LIST_WRITE(list)            do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_WRITE, 0); \
    } else Disable(); } while(0)

#define EXEC_LOCK_LIST_READ(list)             do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ObtainSystemLock(list, SPINLOCK_MODE_READ, 0); \
    } else Forbid(); } while(0)

#define EXEC_UNLOCK_LIST(list)                do { \
    struct ExecLockBase *b; \
    if ((b = PrivExecBase(SysBase)->ExecLockBase)) { \
        b->ReleaseSystemLock(list, 0); \
    } else Permit(); } while(0)

#else

#define EXEC_LOCK_LIST_READ_AND_FORBID(list)     do { Forbid(); } while(0)
#define EXEC_LOCK_LIST_READ_AND_DISABLE(list)    do { Disable(); } while(0)
#define EXEC_LOCK_LIST_WRITE_AND_FORBID(list)     do { Forbid(); } while(0)
#define EXEC_LOCK_LIST_WRITE_AND_DISABLE(list)    do { Disable(); } while(0)
#define EXEC_UNLOCK_LIST_AND_PERMIT(list)   do { Permit(); } while(0)
#define EXEC_UNLOCK_LIST_AND_ENABLE(list)   do { Enable(); } while(0)
#define EXEC_LOCK_LIST_READ(list) /* eps */
#define EXEC_LOCK_LIST_WRITE(list) /* eps */
#define EXEC_UNLOCK_LIST(list) /* eps */
#endif

#endif /* _EXEC_LOCKS_H */