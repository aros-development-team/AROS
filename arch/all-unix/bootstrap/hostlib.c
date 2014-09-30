/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dlfcn.h>
#include <stdio.h>

#include "hostlib.h"

static inline void GetErrorStr(char **error)
{
    if (error)
	*error = (char *)dlerror();
}

void *Host_HostLib_Open(const char *filename, char **error)
{
    void *ret = dlopen(filename, RTLD_NOW);

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
