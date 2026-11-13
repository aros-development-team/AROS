/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/*****************************************************************************

    NAME */
        AROS_LH1(BOOL, MUI_DeleteCustomClass,

/*  SYNOPSIS */
        AROS_LHA(struct MUI_CustomClass *, mcc, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 19, MUIMaster)

/*  FUNCTION
        Attempt to free a custom class. If intuition.library/FreeClass()
        succeeds, free the descriptor and call MUI_FreeClass() on the
        stored superclass.

    INPUTS
        mcc - Descriptor returned by MUI_CreateCustomClass(), or NULL.

    RESULT
        TRUE if the class and descriptor were freed. FALSE if mcc is NULL
        or FreeClass() refuses to free the class, for example because
        instances or subclasses remain.

    NOTES
        On FALSE for a non-NULL descriptor, the descriptor remains allocated.
        On TRUE, do not use the descriptor or its class again.

    SEE ALSO
        MUI_CreateCustomClass(), MUI_FreeClass(), intuition.library/FreeClass()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (mcc)
    {
        Class *super = mcc->mcc_Super;
        
        if (FreeClass(mcc->mcc_Class))
        {
            mui_free(mcc);

            MUI_FreeClass(super);

            return TRUE;
        }
    }

    return FALSE;

    AROS_LIBFUNC_EXIT

} /* MUIA_DeleteCustomClass */
