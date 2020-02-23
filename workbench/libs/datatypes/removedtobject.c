/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#define USE_BOOPSI_STUBS
#include <proto/alib.h>
#include <intuition/intuition.h>
#include <intuition/classusr.h>
#include <datatypes/datatypesclass.h>
#include <clib/boopsistubs.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include "datatypes_intern.h"


/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

        AROS_LH2(LONG, RemoveDTObject,

/*  SYNOPSIS */
        AROS_LHA(struct Window *, window, A0),
        AROS_LHA(Object        *, object, A1),

/*  LOCATION */
        struct Library *, DataTypesBase, 16, DataTypes)

/*  FUNCTION

    Remove an object from the specified window's object list; this will wait
    until the AsyncLayout process is ready. The object will receive a message
    of type DTM_REMOVEDTOBJECT as a sign of it having been removed.

    INPUTS

    window  --  pointer to the window in question
    object  --  pointer to the object to remove

    RESULT

    The position of the object in the list before it was removed; if the
    object wasn't found -1 is returned.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    AddDTObject(), intuition.library/RemoveGList()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    LONG retval = 0;
   
    if(object != NULL)
    {
        struct DTSpecialInfo *dtsi = ((struct Gadget *)object)->SpecialInfo;
        
        while(dtsi->si_Flags & DTSIF_LAYOUTPROC)
        {
            Delay(50);
        }
                
        retval = RemoveGList(window, (struct Gadget *)object, 1);

        DoMethod(object, DTM_REMOVEDTOBJECT, 0);
    }
    
    return retval;

    AROS_LIBFUNC_EXIT
} /* RemoveDTObject */
