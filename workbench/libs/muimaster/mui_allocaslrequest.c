/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH2(APTR, MUI_AllocAslRequest,

/*  SYNOPSIS */
        AROS_LHA(ULONG, reqType, D0),
        AROS_LHA(struct TagItem *, tagList, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 8, MUIMaster)

/*  FUNCTION
        Allocate an ASL requester by forwarding reqType and tagList to
        asl.library/AllocAslRequest(). No MUI-specific attributes are added.

    INPUTS
        reqType - ASL_FileRequest, ASL_FontRequest, or ASL_ScreenModeRequest.
        tagList - Requester-specific ASL tags terminated by TAG_DONE, or
                  NULL to use defaults.

    RESULT
        The allocated requester, or NULL. In the AROS ASL implementation,
        allocation failure sets IoErr() to ERROR_NO_FREE_STORE; an unsupported
        requester type returns NULL with IoErr() set to zero.

    NOTES
        MUI_AllocAslRequestTags() is the variadic tag-list wrapper.

        Run the requester with MUI_AslRequest() and release it with
        MUI_FreeAslRequest(). Refer to ASL for the lifetime of tag payloads
        and requester-specific result fields.

    SEE ALSO
        MUI_AslRequest(), MUI_FreeAslRequest(),
        asl.library/AllocAslRequest()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    return AllocAslRequest(reqType, tagList);

    AROS_LIBFUNC_EXIT

} /* MUIA_AllocAslRequest */
