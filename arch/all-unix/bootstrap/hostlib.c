/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.
*/

#include <dlfcn.h>
#include <stdio.h>

#include <time.h>

#include "hostlib.h"

#define D(x)

static inline void GetErrorStr(char **error)
{
    if (error)
        *error = (char *)dlerror();
}

void *Host_HostLib_Open(const char *filename, char **error)
{
    void *ret;

    D(fprintf(stderr, "[hostlib:unix] Opening '%s'\n", filename);)
    ret = dlopen(filename, RTLD_NOW|RTLD_GLOBAL);
    D(fprintf(stderr, "[hostlib:unix] opened @ 0x%p\n", ret);)

    GetErrorStr(error);

    return ret;
}

int Host_HostLib_Close(void *handle, char **error)
{
    int ret = dlclose(handle);

    GetErrorStr(error);
    return ret;
}

void *Host_HostLib_GetPointer(void *handle, const char *symbol, char **error)
{
    void *ret = dlsym(handle, symbol);

    GetErrorStr(error);
    return ret;
}

int Host_HostLib_GetTime(int _id, uint64_t *seconds, uint64_t *ns)
{
    struct timespec ts;  // HOST timespec, 32 bits or (likely) 64 bits
    clockid_t clk_id = (clockid_t)_id;
    int result = clock_gettime(clk_id, &ts);
    *seconds = ts.tv_sec;
    *ns = ts.tv_nsec;
    return result;
}
