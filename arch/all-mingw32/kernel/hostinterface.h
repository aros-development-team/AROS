/* The mingw interface logic requires that the hostlib version matches
 * HOSTINTERFACE_VERSION exactly.
 */
#define HOSTINTERFACE_VERSION 4

#ifdef __x86_64__
#define __aros __attribute__((sysv_abi))
#else
#define __aros
#endif

struct HostInterface
{
    char *System;
    unsigned int Version;

    void *(__aros *hostlib_Open)(const char *, char**);
    int   (__aros *hostlib_Close)(void *, char **);
    void *(__aros *hostlib_GetPointer)(void *, const char *, char **);
    void  (__aros *hostlib_FreeErrorStr)(char *);
    int   (__aros *KPutC)(int c);
    void  (__aros *Reboot)(unsigned char warm);
};
