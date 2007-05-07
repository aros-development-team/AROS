/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: FreeConfigDev() - Free a ConfigDev structure.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH1(void, FreeConfigDev,

/*  SYNOPSIS */
	AROS_LHA(struct ConfigDev *, configDev, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 14, Expansion)

/*  FUNCTION
	This function will free a ConfigDev structure, as allocated
	by the AllocConfigDev() function.

    INPUTS
	configDev   -   The ConfigDev to free.

    RESULT
	The memory will be returned to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AllocConfigDev()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if(configDev)
	FreeMem(configDev, sizeof(struct ConfigDev));

    AROS_LIBFUNC_EXIT
} /* FreeConfigDev */
