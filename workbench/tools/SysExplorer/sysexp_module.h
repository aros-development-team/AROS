#ifndef SYSEXPLORER_MODULE_H
#define SYSEXPLORER_MODULE_H

#include <exec/nodes.h>

struct SysexpBase;

typedef void (*SYSEXPMODULE_STARTUP)(struct SysexpBase *SysexpBase);

struct SysexpModule
{
    struct Node                 sem_Node;
    SYSEXPMODULE_STARTUP        sem_Startup;
    SYSEXPMODULE_STARTUP        sem_Shutdown;
};

#endif /* SYSEXPLORER_MODULE_H */