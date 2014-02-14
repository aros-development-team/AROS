/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Expansion Resident and initialization.
    Lang: english
*/

#include <aros/config.h>
#include <aros/symbolsets.h>
#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "expansion_intern.h"
#include LC_LIBDEFS_FILE


static int ExpansionInit(LIBBASETYPEPTR LIBBASE)
{
    D(bug("expansion init\n"));

    NEWLIST(&LIBBASE->MountList);
    NEWLIST(&LIBBASE->BoardList);

    InitSemaphore(&LIBBASE->BindSemaphore);
    InitSemaphore(&LIBBASE->BootSemaphore);

    /* See what expansion hardware we can detect. */
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    ConfigChain((APTR)E_EXPANSIONBASE);
#endif

    return TRUE;
}

ADD2INITLIB(ExpansionInit, 0);
