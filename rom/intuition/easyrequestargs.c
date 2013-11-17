/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    Copyright © 2001-2003, The MorphOS Development Team. All Rights Reserved.
    $Id$
*/

#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <exec/types.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>

        AROS_LH4(LONG, EasyRequestArgs,

/*  SYNOPSIS */
        AROS_LHA(struct Window     *, window, A0),
        AROS_LHA(struct EasyStruct *, easyStruct, A1),
        AROS_LHA(ULONG             *, IDCMP_ptr, A2),
        AROS_LHA(APTR               , argList, A3),

/*  LOCATION */
        struct IntuitionBase *, IntuitionBase, 98, Intuition)

/*  FUNCTION
        Opens and handles a requester, which provides one or more choices.
        It blocks the application until the user closes the requester.
        Returned is an integer indicating which gadget had been selected.

    INPUTS
        Window - A reference window. If NULL, the requester opens on
            the default public screen.
        easyStruct - The EasyStruct structure (<intuition/intuition.h>)
            describing the requester.
        IDCMP_Ptr - Pointer to IDCMP flags. The requester will be closed early
            if any of the specified message types is received. This is useful
            for requesters that want to listen to disk changes etc. The
            contents of this pointer is set to the IDCMP flag that caused the
            requester to close. This pointer may be NULL.
        ArgList - The arguments for easyStruct->es_TextFormat.

    RESULT
        -1, if one of the IDCMP flags of IDCMP_ptr was set.
         0, if the rightmost button was clicked or an error occured.
         n, if the n-th button from the left was clicked.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        BuildEasyRequestArgs()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Window *req;
    LONG    	   result;

    req = BuildEasyRequestArgs(window, easyStruct,
                               IDCMP_ptr != NULL ? *IDCMP_ptr : 0, argList);

    /* req = 0/1 is handled by SysReqHandler */
    while ((result = SysReqHandler(req, IDCMP_ptr, TRUE)) == -2)
    {
    }


    FreeSysRequest(req);

    return result;
    
    AROS_LIBFUNC_EXIT
} /* EasyRequestArgs */
