/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.1  1996/08/28 17:55:34  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <intuition/classes.h>
#include <clib/alib_protos.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
	#include <intuition/classusr.h>
	#include <clib/intuition_protos.h>

	__AROS_LH1(void, DisposeObject,

/*  SYNOPSIS */
	__AROS_LHA(APTR, object, A0),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 107, Intuition)

/*  FUNCTION
	Deletes a BOOPSI object. All memory associated with the object
	is freed. The object must have been created with NewObject().
	Some object contain other object which might be freed as well
	when this function is used on the "parent" while others might
	also contain children but won't free them. Read the documentation
	of the class carefully to find out how it behaves.

    INPUTS
	object - The result of a call to NewObject() or a similar function.

    RESULT
	None.

    NOTES
	This functions sends OM_DISPOSE to the oejct.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs((), GetAttr(), MakeClass()
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    __AROS_FUNC_INIT
    __AROS_BASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG MethodID = OM_DISPOSE;

    OCLASS (object)->cl_ObjectCount --;

    DoMethodA (object, (Msg)&MethodID);

    __AROS_FUNC_EXIT
} /* DisposeObject */
