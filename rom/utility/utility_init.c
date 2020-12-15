/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Utility Resident and initialization.
    Lang: english
*/

#include <aros/symbolsets.h>

#include "intern.h"
#include LC_LIBDEFS_FILE

static int UtilityInit(LIBBASETYPEPTR LIBBASE)
{
    GetIntUtilityBase(LIBBASE)->ub_LastID = 0;

    /*
	I no longer allocate memory here for the global namespace, since
	that is not quite legal. (AllocMem is not Forbid() protected).

	Also makes this a little bit shorter. (In time and length).
    */
    InitSemaphore(&GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_Lock);
    NEWLIST((struct List *)&GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_List);
    GetIntUtilityBase(LIBBASE)->ub_NameSpace.ns_Flags = NSF_NODUPS;

    return TRUE;
}

ADD2INITLIB(UtilityInit, 0);
