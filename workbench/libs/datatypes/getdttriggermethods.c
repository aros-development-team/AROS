/*
    (C) 1999 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/
#include "datatypes_intern.h"
#include <proto/intuition.h>
#include <intuition/classusr.h>
#include <datatypes/datatypesclass.h>

/*****************************************************************************

    NAME */
#include <proto/datatypes.h>

	AROS_LH1(struct DTMethods *, GetDTTriggerMethods,

/*  SYNOPSIS */
	AROS_LHA(Object *, object, A0),

/*  LOCATION */
	struct Library *, DTBase, 18, DataTypes)

/*  FUNCTION

    Get a list of the trigger methods an object supports.

    INPUTS

    object  --  pointer to a data type object

    RESULT

    A pointer to a NULL terminated DTMethod list. This list in only valid
    until the object is disposed of.

    NOTES

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

    struct DTMethods *dtm = NULL;
    struct opGet      opGet;
    
    opGet.MethodID    = OM_GET;
    opGet.opg_AttrID  = DTA_TriggerMethods;
    opGet.opg_Storage = (ULONG *)&dtm;
    
    if(!DoMethodA(object, (Msg)&opGet))
	dtm = NULL;
    
    return dtm;

    AROS_LIBFUNC_EXIT
} /* GetDTTriggerMethods */
