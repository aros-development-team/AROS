#define HOSTINTERFACE_VERSION 1

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*hostlib_Open)(const char *, char **);
    int   (*hostlib_Close)(void *, char **);
    void *(*hostlib_GetPointer)(void *, const char *, char **);
    int   (*VKPrintF)(const char *, va_list);
    void  (*Reboot)(unsigned char action);
    struct MinList **ModListPtr;
};
