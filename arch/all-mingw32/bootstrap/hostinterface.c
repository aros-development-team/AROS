#include <stdarg.h>
#include <stdio.h>

#include "../kernel/hostinterface.h"

#include "hostlib.h"
#include "shutdown.h"

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
static struct HostInterface _HostIFace = {
    HOSTINTERFACE_VERSION,
    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    Host_HostLib_FreeErrorStr,
    vprintf,
    Host_Shutdown
};

void *HostIFace = &_HostIFace;
