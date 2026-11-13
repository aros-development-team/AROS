/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
        AROS_LH1(VOID, MUI_DisposeObject,

/*  SYNOPSIS */
        AROS_LHA(Object *, obj, A0),

/*  LOCATION */
        struct Library *, MUIMasterBase, 6, MUIMaster)

/*  FUNCTION
        Dispose of a MUI object, then release its class through MUI_FreeClass().
        The object's OM_DISPOSE implementation determines which owned
        children and other resources are released.

    INPUTS
        obj - Object to dispose of, or NULL to do nothing.

    RESULT
        None.

    NOTES
        The object pointer is invalid after disposal. Remove a child from
        its owner before disposing of it independently. Do not separately
        dispose of children that the parent has already disposed of.

    SEE ALSO
        MUI_NewObjectA(), MUI_FreeClass(), intuition.library/DisposeObject()

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    if (obj == NULL)
        return;
    
    Class *cl = OCLASS(obj);

    DisposeObject(obj);

    MUI_FreeClass(cl);

    AROS_LIBFUNC_EXIT

} /* MUIA_DisposeObject */
