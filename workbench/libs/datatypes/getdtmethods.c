/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

#include "datatypes_intern.h"
#include <proto/intuition.h>
#include <intuition/classusr.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(ULONG, GetDTMethods,

/*  SYNOPSIS */
	AROS_LHA(Object *, object, A0),

/*  LOCATION */
	struct Library *, DTBase, 17, DataTypes)

/*  FUNCTION

    Get a list of the methods an object supports.

    INPUTS

    object   --  pointer to a data type object

    RESULT

    Pointer to a ULONG array which is terminated ~0; the array is only
    valid until the object is disposed of.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    GetDTTriggerMethods()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ULONG retval = NULL;
   
    struct opGet opGet;
   
    opGet.MethodID    = OM_GET;
    opGet.opg_AttrID  = DTA_Methods;
    opGet.opg_Storage = &retval;
    
    if(!DoMethodA(object, (Msg)&opGet))
	retval = NULL;
   
    return retval;

    AROS_LIBFUNC_EXIT
} /* GetDTMethods */
