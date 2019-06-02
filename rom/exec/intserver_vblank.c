/*
    Copyright © 1995-2017, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>

#include <aros/asmcall.h>
#include <exec/execbase.h>
#include <exec/lists.h>

#define AROS_NO_ATOMIC_OPERATIONS
#include <exec_platform.h>

#include "intservers.h"

/* VBlankServer. The same as general purpose IntServer but also counts task's quantum */
AROS_INTH3(VBlankServer, struct List *, intList, intMask, custom)
{
    AROS_INTFUNC_INIT

    D(bug("[Exec] %s()\n", __func__));

    /* First decrease Elapsed time for current task */
    if (SCHEDELAPSED_GET && (--SCHEDELAPSED_GET == 0))
    {
        FLAG_SCHEDQUANTUM_SET;
        FLAG_SCHEDSWITCH_SET;
    }

    /* Chain to the generic routine */
    return AROS_INTC3(IntServer, intList, intMask, custom);

    AROS_INTFUNC_EXIT
}
