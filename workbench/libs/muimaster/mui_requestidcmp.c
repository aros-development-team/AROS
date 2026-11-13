/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <clib/alib_protos.h>
#include <proto/muimaster.h>

#include <proto/intuition.h>
#include <intuition/classusr.h>

#include "muimaster_intern.h"
#include "mui.h"

/*****************************************************************************

    NAME */
        AROS_LH2(VOID, MUI_RequestIDCMP,

/*  SYNOPSIS */
        AROS_LHA(Object *, obj  , A0),
        AROS_LHA(ULONG   , flags, D0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 15, MUIMaster)

/*  FUNCTION
        Add IDCMP event bits to an Area-derived object's legacy input handler.
        If the object is set up and has a window, remove the existing
        handler, update its event mask, and register the resulting handler.
        Otherwise, store the bits for subsequent setup.

    INPUTS
        obj - Non-NULL Area-derived object.
        flags - IDCMP event bits to add; existing bits are retained.

    RESULT
        None.

    NOTES
        Matching events use the legacy MUIM_HandleInput delivery path.
        Area setup registers the stored handler; cleanup removes it.
        For new class implementations, prefer MUIM_Window_AddEventHandler
        with MUIM_HandleEvent. Call in the object's owning GUI task.

    SEE ALSO
        MUI_RejectIDCMP()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    /* Use the Eventhandler to simulate a MUIM_HandleInput */
    if (muiRenderInfo(obj) && (_flags(obj) & MADF_SETUP) && _win(obj))
    {
        if (muiAreaData(obj)->mad_hiehn.ehn_Events)
        {
            DoMethod(_win(obj), MUIM_Window_RemEventHandler,
                (IPTR)&muiAreaData(obj)->mad_hiehn);
        }
        muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;
        if (muiAreaData(obj)->mad_hiehn.ehn_Events)
        {
            DoMethod(_win(obj), MUIM_Window_AddEventHandler,
                (IPTR)&muiAreaData(obj)->mad_hiehn);
        }
    }
    else
    {
        muiAreaData(obj)->mad_hiehn.ehn_Events |= flags;
    }

    AROS_LIBFUNC_EXIT

} /* MUIA_RequestIDCMP */
