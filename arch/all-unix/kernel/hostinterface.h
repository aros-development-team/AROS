#define HOSTINTERFACE_VERSION 3

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char **);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    int   (*KPutC)(int chr);
    struct MinList **ModListPtr;
};
