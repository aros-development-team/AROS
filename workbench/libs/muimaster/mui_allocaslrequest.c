/*
    Copyright � 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(APTR, MUI_AllocAslRequest,

/*  SYNOPSIS */
        AROS_LHA(unsigned long, reqType, D0),
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 8, MUIMaster)

/*  FUNCTION
        Interface to asl.library.

    INPUTS
        see asl.library/AllocAslRequest()

    RESULT
        Pointer to AslRequest

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        asl.library/AllocAslRequest()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AllocAslRequest(reqType, tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AllocAslRequest */
