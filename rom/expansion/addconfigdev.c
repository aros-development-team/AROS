/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Add a ConfigDev struct to the system.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <libraries/configvars.h>
#include <proto/expansion.h>

	AROS_LH1(void, AddConfigDev,

/*  SYNOPSIS */
	AROS_LHA(struct ConfigDev *, configDev, A0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 5, Expansion)

/*  FUNCTION
	This function will add a ConfigDev structure to the systems
	list of Configuration Devices. This function is not normally
	called by user code.

    INPUTS
	configDev   -   The Configuration Device to add to the system.

    RESULT
	The device will be added to the system.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	RemConfigDev()

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
	AddTail(&IntExpBase(ExpansionBase)->BoardList,
            (struct Node *)configDev);
	ReleaseConfigBinding();
    }

    AROS_LIBFUNC_EXIT
} /* AddConfigDev */
