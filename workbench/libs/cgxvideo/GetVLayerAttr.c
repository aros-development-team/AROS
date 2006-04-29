/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH2(ULONG, GetVLayerAttr,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),
	AROS_LHA(ULONG, AttrNum, D0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 9, Cgxvideo)

/*  FUNCTION
	Gets a certain attribute from a given video layer. You have to call
	LockVLayer() to make sure that the result is valid !

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

	AttrNum - attribute that you want to get

    RESULT
	value - the value for the given attribute

    NOTES
	Attributes available are:

	VOA_BaseAddress - 	if this attribute is specified the base address for
				the source data is returned

	VOA_ColorKeyPen - 	returns the pen number used for color keying. If color
				keying is not enabled, -1 is returned

	VOA_ColorKey -	returns the 24 bit color value used for color keying.
				If color keying is not enabled, -1 is returned.

    EXAMPLE

    BUGS

    SEE ALSO
	SetVLayerAttr()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)

    aros_print_not_implemented ("GetVLayerAttr");

    AROS_LIBFUNC_EXIT
} /* GetVLayerAttr */
