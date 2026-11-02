/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include <proto/asl.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, MUI_FreeAslRequest,

/*  SYNOPSIS */
        AROS_LHA(APTR, requester, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 10, MUIMaster)

/*  FUNCTION
        Release an allocated ASL requester through asl.library/FreeAslRequest().
        ASL also releases the requester-specific result storage it owns.

    INPUTS
        requester - Requester allocated by MUI_AllocAslRequest() or
                    asl.library/AllocAslRequest(), or NULL to do nothing.

    RESULT
        None.

    NOTES
        Call after requester execution has finished. Do not use the requester
        or its owned result storage after this call.

    SEE ALSO
        MUI_AllocAslRequest(), MUI_AslRequest(), asl.library/FreeAslRequest()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    FreeAslRequest(requester);

    AROS_LIBFUNC_EXIT

} /* MUIA_FreeAslRequestA */
