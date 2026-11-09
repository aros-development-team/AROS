/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(BOOL, MUI_AslRequest,

/*  SYNOPSIS */
        AROS_LHA(APTR, requester, A0),
        AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
        struct Library *, MUIMasterBase, 9, MUIMaster)

/*  FUNCTION
        Run an allocated ASL requester by forwarding requester and tagList
        to asl.library/AslRequest(). The call runs the ASL event loop and
        waits for the requester to finish.

    INPUTS
        requester - Requester allocated by MUI_AllocAslRequest() or
                    asl.library/AllocAslRequest().
        tagList - Additional requester-specific ASL tags terminated by
                  TAG_DONE, or NULL to retain the existing settings.

    RESULT
        The BOOL result of AslRequest(): TRUE on successful completion,
        FALSE on cancellation or failure. NULL requester returns FALSE
        and sets IoErr() to ERROR_NO_FREE_STORE in AROS.

    NOTES
        MUI_AslRequestTags() is the variadic tag-list wrapper.

        Call from a process context suitable for an ASL requester. Result
        fields and detailed error behavior belong to the ASL requester
        type. The requester remains allocated after this call.

    SEE ALSO
        MUI_AllocAslRequest(), MUI_FreeAslRequest(),
        asl.library/AslRequest()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AslRequest(requester, tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AslRequest */
