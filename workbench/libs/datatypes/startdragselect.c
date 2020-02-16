/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */

        AROS_LH1(ULONG, StartDragSelect,

/*  SYNOPSIS */
        AROS_LHA(Object *, o, A0),

/*  LOCATION */
        struct Library *, DataTypesBase, 50, DataTypes)

/*  FUNCTION

    Start drag-selection by the user; the drag selection will only start
    if the object in question supports DTM_SELECT, is in a window or
    requester and no layout-process is working on the object.

    INPUTS

    o   --  data type object in question; may be NULL

    RESULT

    TRUE if all went OK, FALSE otherwise. If FALSE, IoErr() gives further
    information:

    ERROR_ACTION_NOT_KNOWN   --  the object doesn't support DTM_SELECT
    ERROR_OBJECT_IN_USE      --  the object is currently occupied

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

    6.8.99  SDuvan  implemented

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DTSpecialInfo *dtsi;
    BOOL                  retval = TRUE;

    if(o == NULL)
        return FALSE;

    dtsi = ((struct Gadget *)o)->SpecialInfo;

    /* Doesn't support drag selection? */
    if(FindMethod(GetDTMethods(o), DTM_SELECT) == NULL)
    {
        SetIoErr(ERROR_ACTION_NOT_KNOWN);
        return FALSE;
    }
    
    ObtainSemaphore(&dtsi->si_Lock);

    if(dtsi->si_Flags & DTSIF_LAYOUTPROC)
    {
        retval = FALSE;
        SetIoErr(ERROR_OBJECT_IN_USE);
    }
    else
        dtsi->si_Flags |= DTSIF_DRAGSELECT;

    ReleaseSemaphore(&dtsi->si_Lock);

    return retval;

    AROS_LIBFUNC_EXIT
} /* StartDragSelect */

