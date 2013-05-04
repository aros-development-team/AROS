/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	AROS_LH1(void, UnlockVLayer,

/*  SYNOPSIS */
	AROS_LHA(struct VLayerHandle *, VLayerHandle, A0),

/*  LOCATION */
	struct Library *, CGXVideoBase, 11, Cgxvideo)

/*  FUNCTION
	Unlocks a previouly locked video layer

    INPUTS
	VLayerHandle - pointer to a previously created videolayer handle

    RESULT
	none

    NOTES

    EXAMPLE

    BUGS
        This function is unimplemented.

    SEE ALSO
	LockVLayer()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    aros_print_not_implemented ("UnlockVLayer");

    AROS_LIBFUNC_EXIT
} /* UnlockVLayer */
