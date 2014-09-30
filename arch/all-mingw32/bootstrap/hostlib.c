/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <windows.h>

#include "hostlib.h"

#define D(x)

static void GetErrorStr(char **error, BOOL condition)
{
    if (error != NULL) {
    	if (condition) {
	    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)error, 0, NULL );
	} else
	    *error = NULL;
    }
}

void __aros Host_HostLib_FreeErrorStr(char *error)
{
    LocalFree(error);
}

void * __aros Host_HostLib_Open(const char *filename, char **error)
{
    HMODULE handle;

    D(printf("[hostlib] Open: filename=%s\n", filename));
    handle = LoadLibrary(filename);
    GetErrorStr(error, !handle);

    return handle;
}

int __aros Host_HostLib_Close(void *handle, char **error)
{
    int err;

    D(printf("[hostlib] Close: handle=0x%p\n", handle));
    err = !FreeLibrary(handle);
    GetErrorStr(error, err);

    return err;
}

void * __aros Host_HostLib_GetPointer(void *handle, const char *symbol, char **error)
{
    void *ptr;

    D(printf("[hostlib] GetPointer: handle=0x%p, symbol=%s\n", handle, symbol));
    ptr = GetProcAddress(handle, symbol);
    GetErrorStr(error, !ptr);
    return ptr;
}

