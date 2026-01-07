/*
    Copyright (C) 2023-2026, The AROS Development Team. All rights reserved

    Desc: Generic pcixhci init code for AROS
*/

#include <aros/bootloader.h>
#include <aros/symbolsets.h>
#include <exec/types.h>

#include <proto/exec.h>

#include <string.h>

#include "pcixhci.h"

/*
 * Process some AROS-specific arguments.
 */
static int getArguments(struct PCIDevice *base)
{
#if defined(AROS_USE_LOGRES)
#ifdef LogResBase
#undef LogResBase
#endif
#ifdef LogResHandle
#undef LogResHandle
#endif
    APTR LogResBase;
#define LogHandle (base->hd_LogRHandle)
    base->hd_LogResBase = OpenResource("log.resource");
    if(base->hd_LogResBase) {
        LogResBase = base->hd_LogResBase;
        base->hd_LogRHandle = logInitialise(&base->hd_Device.dd_Library.lib_Node);
    }
#endif
    return TRUE;
}

ADD2INITLIB(getArguments, 10)
