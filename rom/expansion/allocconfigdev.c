/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: AllocConfigDev() - Create a new ConfigDev in a compatible way.
    Lang: english
*/
#include "expansion_intern.h"
#include <exec/memory.h>
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH0(struct ConfigDev *, AllocConfigDev,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 8, Expansion)

/*  FUNCTION
	AllocConfigDev() will allocate a new ConfigDev structure for
	you. You should use this function in order for you to be
	compatible with future versions of the OS in case this
	structure changes.

    INPUTS
	None.

    RESULT
	A newly created ConfigDev structure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeConfigDev()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct ExpansionBase *,ExpansionBase)

    return AllocMem(sizeof(struct ConfigDev), MEMF_CLEAR|MEMF_PUBLIC);

    AROS_LIBFUNC_EXIT
} /* AllocConfigDev */
