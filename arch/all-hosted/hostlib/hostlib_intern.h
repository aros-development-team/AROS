#ifndef HOSTLIB_INTERN_H
#define HOSTLIB_INTERN_H

#include <exec/nodes.h>
#include <exec/semaphores.h>

/*
 * Windows is a very harsh environment.
 * It requires us to Forbid() in order to call itself.
 *
 * Darwin needs the same: host libc locks (environ, locale, malloc) are
 * shared with the host process' own threads. A guest task preempted in the
 * middle of a host call keeps such a lock in its suspended context, and a
 * host thread blocking on it deadlocks the whole process. Forbid() makes
 * host calls run to completion. This costs nothing on the single-CPU
 * guest: while the AROS host thread is inside a host call no other guest
 * task can execute anyway.
 */
#if defined(HOST_OS_mingw32) || defined(HOST_OS_darwin)
#define USE_FORBID_LOCK
#endif

struct HostLibBase 
{
    struct Node hlb_Node;
    struct HostInterface *HostIFace;
#ifndef USE_FORBID_LOCK
    struct SignalSemaphore HostSem;
#endif
};

#if defined(USE_FORBID_LOCK)
#define HOSTLIB_LOCK()   Forbid()
#define HOSTLIB_UNLOCK() Permit()
#else
#define HOSTLIB_LOCK()   ObtainSemaphore(&HostLibBase->HostSem)
#define HOSTLIB_UNLOCK() ReleaseSemaphore(&HostLibBase->HostSem)
#endif

#endif
