/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <proto/intuition.h>
#include <intuition/classusr.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(ULONG *, GetDTMethods,

/*  SYNOPSIS */
	AROS_LHA(Object *, object, A0),

/*  LOCATION */
	struct Library *, DataTypesBase, 17, DataTypes)

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

    ULONG *retval = NULL;
   
    struct opGet opGet;
   
    if(object == NULL)
	return NULL;

    opGet.MethodID    = OM_GET;
    opGet.opg_AttrID  = DTA_Methods;
    opGet.opg_Storage = (IPTR *)&retval;
    
    if(!DoMethodA(object, (Msg)&opGet))
	retval = NULL;
   
    return retval;

    AROS_LIBFUNC_EXIT
} /* GetDTMethods */
