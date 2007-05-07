/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Release the lock on the CurrentBinding data.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH0(void, ReleaseConfigBinding,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 21, Expansion)

/*  FUNCTION
	This function will release the lock obtained by
	ObtainCurrentBinding(). It will release the SignalSemaphore,
	and allow others to bind to drivers.

    INPUTS
	None.

    RESULT
	None.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ObtainCurrentBinding()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ReleaseSemaphore(&IntExpBase(ExpansionBase)->eb_BindSemaphore);

    AROS_LIBFUNC_EXIT
} /* ReleaseConfigBinding */
