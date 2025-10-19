#ifndef _HOSTINTERFACE_H
#define _HOSTINTERFACE_H

#include <stdint.h>

#define HOSTINTERFACE_VERSION 4

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char **);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    int   (*KPutC)(int chr);
    int   (*host_GetTime)(int, uint64_t *, uint64_t *);
    struct MinList **ModListPtr;
};

#endif /* !_HOSTINTERFACE_H */
