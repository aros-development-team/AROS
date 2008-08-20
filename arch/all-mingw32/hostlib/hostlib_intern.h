#ifndef HOSTLIB_INTERN_H
#define HOSTLIB_INTERN_H

#ifdef __AROS__

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

struct HostLibBase {
    struct Node hlb_Node;
};

#endif

struct HostLibInterface
{
    void *(*HostLib_Open)(const char *, char**);
    int (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    void (*HostLib_FreeErrorStr)(char *);
    unsigned long (*HostLib_GetInterface)(void *, char **, void **);
};

#endif
