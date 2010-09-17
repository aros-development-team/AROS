#include <proto/exec.h>

#include <stdarg.h>

#include "hostinterface.h"
#include "kernel_cpu.h"
#include "kernel_mingw32.h"

int mykprintf(const char *fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);

    if (SysBase)
        Forbid();
    r = HostIFace->VKPrintF(fmt, args);
    if (SysBase)
        Permit();

    va_end(args);

    return r;
}

int myvkprintf (const char *fmt, va_list args)
{
    int res;
    
    Forbid();
    res = HostIFace->VKPrintF(fmt, args);
    Permit();

    return res;
}

int myrkprintf(const char *foo, const char *bar, int baz, const char* fmt, ...)
{
    va_list args;
    int r;

    va_start(args, fmt);

    Forbid();
    r = HostIFace->VKPrintF(fmt, args);
    Permit();

    va_end(args);

    return r;
}
