#ifndef _HOSTINTERFACE_H
#define _HOSTINTERFACE_H

#define HOSTINTERFACE_VERSION 3

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char **);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    int   (*KPutC)(int chr);
    int   (*host_GetTime)(int, void *);
    struct MinList **ModListPtr;
};

#endif /* !_HOSTINTERFACE_H */
