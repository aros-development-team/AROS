#include <dlfcn.h>
#include <stdio.h>

#include "hostlib.h"

#define D(x)

char *Host_HostLib_GetErrorStr(void)
{
    return dlerror();
}

void *Host_HostLib_Open(const char *filename)
{
    return dlopen(filename, RTLD_NOW);
}

int Host_HostLib_Close(void *handle)
{
    return dlclose(handle);
}

void *Host_HostLib_GetPointer(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

unsigned long Host_HostLib_GetInterface(void *handle, char **names, void **funcs)
{
    unsigned long unresolved = 0;

    for (; *names; names++) {
        *funcs = dlsym(handle, *names);
        D(printf("[hostlib] GetInterface: handle=0x%08x, symbol=%s, value=0x%08x\n", handle, *names, *funcs));
        if (*funcs++ == NULL)
            unresolved++;
    }
    return unresolved;
}
