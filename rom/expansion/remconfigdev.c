/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Remove a ConfigDev structure from the system.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>
/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH1(void, RemConfigDev,

/*  SYNOPSIS */
	AROS_LHA(struct ConfigDev *, configDev, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 18, Expansion)

/*  FUNCTION
	This routine will remove the given ConfigDev from the list
	of Configuration Devices in the system.

    INPUTS
	configDev   -   The ConfigDev structure to remove.

    RESULT
	The ConfigDev structure will be removed from the systems list.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AddConfigDev()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(configDev)
    {
	ObtainConfigBinding();
	Remove((struct Node *)configDev);
	ReleaseConfigBinding();
    }

    AROS_LIBFUNC_EXIT
} /* RemConfigDev */
