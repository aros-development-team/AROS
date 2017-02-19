/*
    Copyright © 2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define __AROS_KERNEL__

#include "exec_intern.h"

#include "kernel_intern.h"

#include "intservers.h"

/*
 * Unlike the VBlankServer, we might not run at a fixed 60Hz.
 */
AROS_INTH3(APICHeartbeatServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

    D(bug("[Kernel:APIC] %s()\n", __func__));

    return 1;

    AROS_INTFUNC_EXIT
}
