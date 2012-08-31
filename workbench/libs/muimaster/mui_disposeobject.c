/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
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
        Deletes MUI object and its child objects.
        
    INPUTS
        obj - pointer to MUI object created with MUI_NewObject. May be NULL,
              in which case this function has no effect.
        
    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS
        MUI will call DisposeObject(), then call CloseLibrary() on
        OCLASS(obj)->h_Data if cl_ID!=NULL && h_Data!=NULL.

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
