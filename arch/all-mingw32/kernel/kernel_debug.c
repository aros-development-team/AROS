#include <proto/exec.h>

#include <stdarg.h>

#include "kernel_base.h"
#include "kernel_debug.h"

int mykprintf(const UBYTE * fmt, ...)
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

int myvkprintf (const UBYTE *fmt, va_list args)
{
    int res;
    
    Forbid();
    res = HostIFace->VKPrintF(fmt, args);
    Permit();

    return res;
}

int myrkprintf(const STRPTR foo, const STRPTR bar, int baz, const UBYTE * fmt, ...)
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
