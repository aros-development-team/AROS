/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Get the CurrentBinding structure.
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <proto/expansion.h>

	AROS_LH2(ULONG, GetCurrentBinding,

/*  SYNOPSIS */
	AROS_LHA(struct CurrentBinding *, currentBinding, A0),
	AROS_LHA(ULONG                  , bindingSize, D0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 23, Expansion)

/*  FUNCTION
	This function will return the contents of the "currentBinding"
	structure. The currentBinding structure may be set with
	SetConfigBinding(). This is how arguments are passed to a newly
	configured device.

	A CurrentBinding structure has the following information:
	-   the name of the currently loaded driver file
	-   the product string associated with this driver
	-   a singly linked list of ConfigDev structures

	You may not need this information, but it is recommended that you
	at least make sure you can deal with the product code in the
	ConfigDev structure.

    INPUTS
	currentBinding  -   a pointer to the CurrentBinding structure that
			    you wish filled in.
	bindingSize     -   the size of the currentBinding structure. Do
			    not pass less than sizeof(struct CurrentBinding).

    RESULT
	The size of the CurrentBinding structure returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	SetCurrentBinding()

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    expansion_lib.fd and clib/expansion_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if( bindingSize > sizeof(struct CurrentBinding) )
	bindingSize = sizeof(struct CurrentBinding);

    CopyMem(&IntExpBase(ExpansionBase)->CurrentBinding,
	    currentBinding,
	    bindingSize);

    return sizeof(struct CurrentBinding);

    AROS_LIBFUNC_EXIT
} /* GetCurrentBinding */
