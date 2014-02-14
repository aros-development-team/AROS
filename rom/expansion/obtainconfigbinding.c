/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Lock the CurrentBinding structure.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH0(void, ObtainConfigBinding,

/*  SYNOPSIS */
	/* void */

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 20, Expansion)

/*  FUNCTION
	ObtainConfigBinding() gives you permission to bind drivers
	to a ConfigDev structure. It exists so that two drivers
	at once do not try and bind the same ConfigDev structures
	at the same time.

	Since most of the data required to bind drivers is statically
	kept, so you must lock out other users from accessing the
	structures at the same time.

	This call is based on the Exec SignalSemaphores, and will
	block until it is safe to proceed.

    INPUTS
	None.

    RESULT
	You will have the lock on the CurrentBindings. Please finish
	as quickly as you can.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ReleaseConfigBinding()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&IntExpBase(ExpansionBase)->BindSemaphore);

    AROS_LIBFUNC_EXIT
} /* ObtainConfigBinding */
