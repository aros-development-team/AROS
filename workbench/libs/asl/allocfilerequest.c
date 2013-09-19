/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/asl.h>
#include "asl_intern.h"
#include <aros/libcall.h>

        AROS_LH0(struct FileRequester *, AllocFileRequest,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct Library *, AslBase, 5, Asl)

/*  FUNCTION
        Obsolete. Use AllocAslRequest() instead.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AllocAslRequest()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return ((struct FileRequester *)AllocAslRequest(ASL_FileRequest, NULL));

    AROS_LIBFUNC_EXIT
} /* AllocFileRequest */
