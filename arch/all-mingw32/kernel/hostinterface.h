struct HostInterface
{
    void *(*HostLib_Open)(const char *, char**);
    int (*HostLib_Close)(void *, char **);
    void *(*HostLib_GetPointer)(void *, const char *, char **);
    void (*HostLib_FreeErrorStr)(char *);
    unsigned long (*HostLib_GetInterface)(void *, char **, void **);
    int (*VKPrintF)(const char *, va_list);
    int (*PutChar)(int c);
};
