/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "cgxvideo_intern.h"

/*****************************************************************************

    NAME */
#include <clib/cgxvideo_protos.h>

	ULONG AttachVLayerTags (

/*  SYNOPSIS */
	struct VLayerHandle * ,
	struct Window *
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
    extern void aros_print_not_implemented (char *);

    aros_print_not_implemented ("AttachVLayerTags");

    AROS_LIBFUNC_EXIT
} /* AttachVLayerTags */
