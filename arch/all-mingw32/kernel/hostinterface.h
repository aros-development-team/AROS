#define HOSTINTERFACE_VERSION 2

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char**);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    void  (*hostlib_FreeErrorStr)(char *);
    int   (*VKPrintF)(const char *, va_list);
    void  (*Reboot)(unsigned char warm);
};
