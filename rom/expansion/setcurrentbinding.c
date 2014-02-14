/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include "expansion_intern.h"
#include <proto/exec.h>

/*****************************************************************************

    NAME */
#include <clib/expansion_protos.h>

	AROS_LH2(void, SetCurrentBinding,

/*  SYNOPSIS */
	AROS_LHA(struct CurrentBinding *, currentBinding, A0),
	AROS_LHA(ULONG                  , bindingSize, D0),

/*  LOCATION */
	struct ExpansionBase *, ExpansionBase, 22, Expansion)

/*  FUNCTION
	This function will return the contents of the "currentBinding"
	structure. The currentBinding structure may be returned with
	GetConfigBinding(). This is how arguments are passed to a newly
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
	The size of the CurrentBinding structure set.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	GetCurrentBinding()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG size = bindingSize;

    if( bindingSize > sizeof(struct CurrentBinding) )
	size = sizeof(struct CurrentBinding);

    CopyMem(currentBinding,
	    &IntExpBase(ExpansionBase)->CurrentBinding,
	    size);

    /* NULL pad the rest */
    while(size < sizeof(struct CurrentBinding))
        ((UBYTE *)&IntExpBase(ExpansionBase)->CurrentBinding)[size++] = 0;

    AROS_LIBFUNC_EXIT
} /* SetCurrentBinding */
