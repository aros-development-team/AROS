/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Expansion Resident and initialization.
    Lang: english
*/

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <exec/execbase.h>
#include <aros/symbolsets.h>
#include <proto/exec.h>
#include <proto/expansion.h>

#include "expansion_intern.h"
#include LC_LIBDEFS_FILE


static int ExpansionInit(LIBBASETYPEPTR LIBBASE)
{
    D(bug("expansion init\n"));

    NEWLIST(&LIBBASE->eb_MountList);
    NEWLIST(&LIBBASE->eb_BoardList);

    memset(&LIBBASE->eb_BindSemaphore, 0, sizeof(LIBBASE->eb_BindSemaphore));
    InitSemaphore(&LIBBASE->eb_BindSemaphore);

    /* See what expansion hardware we can detect. */
#if (AROS_FLAVOUR & AROS_FLAVOUR_BINCOMPAT)
    ConfigChain((APTR)E_EXPANSIONBASE);
#endif

    return TRUE;
}

ADD2INITLIB(ExpansionInit, 0);
