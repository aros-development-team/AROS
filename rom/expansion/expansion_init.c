/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
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


AROS_SET_LIBFUNC(ExpansionInit, LIBBASETYPE, LIBBASE)
{
    AROS_SET_LIBFUNC_INIT

    NEWLIST(&LIBBASE->eb_MountList);

    /* See what expansion hardware we can detect. */
#if 0
    ConfigChain();
#endif

    return TRUE;

    AROS_SET_LIBFUNC_EXIT
}

ADD2INITLIB(ExpansionInit, 0);
