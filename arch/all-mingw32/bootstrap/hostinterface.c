#include <stdarg.h>
#include <stdio.h>

#include "../kernel/hostinterface.h"

#include "hostlib.h"
#include "shutdown.h"

/*
 * Our debug output goes to stderr.
 * bootstrap's file redirection code expects this.
 */
static int __aros KPutC(int chr)
{
    int ret;

    ret = fputc(chr, stderr);
    if (chr == '\n')
        fflush(stderr);

    return ret;
}

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
static struct HostInterface _HostIFace =
{
    "Windows",
    HOSTINTERFACE_VERSION,
    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    Host_HostLib_FreeErrorStr,
    KPutC,
    Host_Shutdown
};

void *HostIFace = &_HostIFace;

