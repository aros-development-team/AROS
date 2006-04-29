/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/
#include <aros/debug.h>

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	ULONG CreateVLayerHandleTags(

/*  SYNOPSIS */
	struct Screen * ,
	Tag  ,
	...  )

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CGXVideoBase)

    aros_print_not_implemented ("CreateVLayerHandleTags");

    AROS_LIBFUNC_EXIT
} /* CreateVLayerHandleTags */
