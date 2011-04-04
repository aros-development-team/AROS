#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <dos/bptr.h>

#include <stdarg.h>
#include <stdio.h>

#include "hostinterface.h"

#include "hostlib.h"
#include "shutdown.h"

#if AROS_MODULES_DEBUG
/* gdb hooks from which it obtains modules list */

/* This is needed in order to bring in definition of struct segment */
#include "../../../rom/kernel/debug_intern.h"

struct segment *seg = NULL;
struct Resident *res = NULL;
struct MinList *Debug_ModList = NULL;
#endif

/*
 * Redirect debug output to stderr. This is especially
 * needed on iOS where reading stdout is only possible with
 * remote gdb, which is tied to XCode's own build system.
 * On other unixes this won't hurt either.
 */
static int VKPrintf(const char *format, va_list ap)
{
    int ret;

    ret = vfprintf(stderr, format, ap);
    /* Sync up with unbuffered output from KrnPutChar() */
    fflush(stderr);

    return ret;
}

static int KPutC(int chr)
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
static struct HostInterface _HostIFace = {
    AROS_ARCHITECTURE,
    HOSTINTERFACE_VERSION,
    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    VKPrintf,
    Host_Shutdown,
#if AROS_MODULES_DEBUG
    &Debug_ModList,
#else
    NULL,
#endif
    KPutC
};

void *HostIFace = &_HostIFace;
