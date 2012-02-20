/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/
#define USE_BOOPSI_STUBS
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <datatypes/datatypesclass.h>
#include <clib/boopsistubs.h>
#include "datatypes_intern.h"

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(struct DTMethod *, GetDTTriggerMethods,

/*  SYNOPSIS */
	AROS_LHA(Object *, object, A0),

/*  LOCATION */
	struct Library *, DataTypesBase, 18, DataTypes)

/*  FUNCTION

    Get a list of the trigger methods an object supports.

    INPUTS

    object  --  pointer to a data type object

    RESULT

    A pointer to a STM_DONE terminated DTMethod list. This list in only valid
    until the object is disposed of.

    NOTES

    Some trigger methods requires an argument (calling these with a NULL
    argument is wrong). Use GetDTTriggerMethodDataFlags() to obtain the
    type of the requested argument.

    EXAMPLE

    To call the specific method, do the following:

    DoMethod(object, DTM_TRIGGER, myMethod);

    BUGS

    SEE ALSO

    GetDTMethods()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct DTMethod *dtm = NULL;
    struct opGet      opGet;
    
    if(object == NULL)
	return NULL;

    opGet.MethodID    = OM_GET;
    opGet.opg_AttrID  = DTA_TriggerMethods;
    opGet.opg_Storage = (IPTR *)&dtm;
    
    if(!DoMethodA(object, (Msg)&opGet))
	dtm = NULL;
    
    return dtm;

    AROS_LIBFUNC_EXIT
} /* GetDTTriggerMethods */
