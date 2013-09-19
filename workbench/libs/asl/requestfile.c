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

        AROS_LH1(BOOL, RequestFile,

/*  SYNOPSIS */
        AROS_LHA(struct FileRequester *, fileReq, A0),

/*  LOCATION */
        struct Library *, AslBase, 7, Asl)

/*  FUNCTION
        Obsolete. Use AslRequest() instead.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        AslRequest()

    INTERNALS

    HISTORY
        27-11-96    digulla automatically created from
                            asl_lib.fd and clib/asl_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return (AslRequest(fileReq, NULL));

    AROS_LIBFUNC_EXIT
} /* RequestFile */
