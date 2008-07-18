#include <stdio.h>
#include <windows.h>

#include "hostlib.h"

#define D(x) x

static void GetErrorStr(char **error, BOOL condition)
{
    if (error != NULL) {
    	if (condition) {
	    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
			  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), error, 0, NULL );
	} else
	    *error = NULL;
    }
}

void Kern_HostLib_FreeErrorStr(char *error)
{
    LocalFree(error);
}

void *Kern_HostLib_Open(const char *filename, char **error)
{
    HMODULE handle;

    D(printf("[hostlib] Open: filename=%s\n", filename));
    handle = LoadLibrary(filename);
    GetErrorStr(error, !handle);

    return handle;
}

int Kern_HostLib_Close(void *handle, char **error)
{
    int err;

    D(printf("[hostlib] Close: handle=0x%08x\n", handle));
    err = !FreeLibrary(handle);
    GetErrorStr(error, err);

    return err;
}

void *Kern_HostLib_GetPointer(void *handle, const char *symbol, char **error)
{
    void *ptr;

    D(printf("[hostlib] GetPointer: handle=0x%08x, symbol=%s\n", handle, symbol));
    ptr = GetProcAddress(handle, symbol);
    GetErrorStr(error, !ptr);
    return ptr;
}

unsigned long Kern_HostLib_GetInterface(void *handle, char **names, void **funcs)
{
    unsigned long unresolved = 0;

    for (; *names; names++) {
        *funcs = GetProcAddress(handle, *names);
        if (*funcs++ == NULL)
            unresolved++;
    }
}
