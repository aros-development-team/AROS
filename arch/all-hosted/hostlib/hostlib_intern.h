#ifndef HOSTLIB_INTERN_H
#define HOSTLIB_INTERN_H

#include <exec/nodes.h>
#include <exec/semaphores.h>

/*
 * Windows is a very harsh environment.
 * It requires us to Forbid() in order to call itself.
 */
#ifdef HOST_OS_mingw32
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

#ifdef USE_FORBID_LOCK
#define HOSTLIB_LOCK()   Forbid()
#define HOSTLIB_UNLOCK() Permit()
#else
#define HOSTLIB_LOCK()   ObtainSemaphore(&HostLibBase->HostSem)
#define HOSTLIB_UNLOCK() ReleaseSemaphore(&HostLibBase->HostSem)
#endif

#endif
