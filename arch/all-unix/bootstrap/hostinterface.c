#include <aros/config.h>
#include <exec/lists.h>

#include <stdarg.h>
#include <stdio.h>

#include "hostinterface.h"

#include "hostlib.h"
#include "shutdown.h"

#if AROS_MODULES_DEBUG
struct MinList *Debug_ModList = NULL;
#endif

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
    vprintf,
    Host_Shutdown,
#if AROS_MODULES_DEBUG
    &Debug_ModList
#else
    NULL
#endif
};

void *HostIFace = &_HostIFace;
