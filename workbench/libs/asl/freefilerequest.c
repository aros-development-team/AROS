/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

/*****************************************************************************

    NAME */
#include <proto/asl.h>
#include <libraries/asl.h>
#include "asl_intern.h"

        AROS_LH1(void, FreeFileRequest,

/*  SYNOPSIS */
        AROS_LHA(struct FileRequester *, fileReq, A0),

/*  LOCATION */
        struct Library *, AslBase, 6, Asl)

/*  FUNCTION
        Obsolete. Use FreeAslRequest() instead.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        FreeAslRequest()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeAslRequest(fileReq);

    return;

    AROS_LIBFUNC_EXIT
} /* FreeFileRequest */
