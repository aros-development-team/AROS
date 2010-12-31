#ifndef HOSTLIB_INTERN_H
#define HOSTLIB_INTERN_H

#include <exec/nodes.h>
#include <exec/semaphores.h>

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

#endif
