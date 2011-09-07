#define HOSTINTERFACE_VERSION 3

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char**);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    void  (*hostlib_FreeErrorStr)(char *);
    int   (*KPutC)(int c);
    void  (*Reboot)(unsigned char warm);
};
