struct HostInterface
{
    void *(*HostLib_Open)(const char *, char**);
    int (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    void (*HostLib_FreeErrorStr)(char *);
    unsigned long (*HostLib_GetInterface)(void *, const char **, void **);
    int (*VKPrintF)(const char *, va_list);
    void (*Reboot)(unsigned char warm);
};
