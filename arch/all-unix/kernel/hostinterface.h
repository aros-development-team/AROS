struct HostInterface
{
    void *(*HostLib_Open)(const char *);
    int (*HostLib_Close)(void *);
    void *(*HostLib_GetPointer)(void *, const char *);
    char *(*HostLib_GetErrorStr)(void);
    unsigned long (*HostLib_GetInterface)(void *, char **, void **);
    int (*VKPrintF)(const char *, va_list);
    int (*PutChar)(int c);
    void (*_Shutdown)(unsigned long action);
};
