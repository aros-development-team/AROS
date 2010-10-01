#define HOSTINTERFACE_VERSION 1

struct HostInterface
{
    unsigned int Version;

    void *(*HostLib_Open)(const char *, char **);
    int   (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *);
    int   (*VKPrintF)(const char *, va_list);
    void  (*Reboot)(unsigned char action);
};
