/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.5  1997/07/21 17:36:08  srittau
    DisposeObject() accepts now NULL pointers.

    Revision 1.4  1997/01/27 00:36:37  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:01  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:18  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/28 17:55:34  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <intuition/classes.h>
#include <proto/alib.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	AROS_LH1(void, DisposeObject,

/*  SYNOPSIS */
	AROS_LHA(APTR, object, A0),

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
	object - The result of a call to NewObject() or a similar function,
	         may be NULL.

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
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct IntuitionBase *,IntuitionBase)
    ULONG MethodID = OM_DISPOSE;

    if (!object)
        return;

    OCLASS (object)->cl_ObjectCount --;

    DoMethodA (object, (Msg)&MethodID);

    AROS_LIBFUNC_EXIT
} /* DisposeObject */
