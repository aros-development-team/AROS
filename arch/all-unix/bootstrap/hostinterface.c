#include <stdarg.h>

#include "hostinterface.h"

#include "debug.h"
#include "hostlib.h"
#include "shutdown.h"

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
static struct HostInterface _HostIFace = {
    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    Host_HostLib_GetInterface,
    Host_VKPrintF,
    Host_PutChar,
    Host_Shutdown
};

void *HostIFace = &_HostIFace;
