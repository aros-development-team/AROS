#define HOSTINTERFACE_VERSION 1

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(*HostLib_Open)(const char *, char **);
    int   (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    int   (*VKPrintF)(const char *, va_list);
    void  (*Reboot)(unsigned char action);
    struct MinList **ModListPtr;
    int   (*KPutC)(int chr);
};
