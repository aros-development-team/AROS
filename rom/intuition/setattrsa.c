/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.4  1997/01/27 00:36:43  ldp
    Polish

    Revision 1.3  1996/12/10 14:00:08  aros
    Moved #include into first column to allow makedepend to see it.

    Revision 1.2  1996/10/24 15:51:24  aros
    Use the official AROS macros over the __AROS versions.

    Revision 1.1  1996/08/28 17:55:36  digulla
    Proportional gadgets
    BOOPSI


    Desc:
    Lang: english
*/
#include <proto/alib.h>
#include "intuition_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	AROS_LH2(ULONG, SetAttrsA,

/*  SYNOPSIS */
	AROS_LHA(APTR            , object, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct IntuitionBase *, IntuitionBase, 108, Intuition)

/*  FUNCTION
	Changes several attributes of an object at the same time. How the
	object interprets the new attributes depends on the class.

    INPUTS
	object - Change the attributes of this object
	tagList - This is a list of attribute/value-pairs

    RESULT
	Depends in the class. For gadgets, this value is non-zero if
	they need redrawing after the values have changed. Other classes
	will define other return values.

    NOTES
	This function sends OM_SET to the object.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), DisposeObject(), GetAttr(), MakeClass(),
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
    struct opSet ops;

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = tagList;
    ops.ops_GInfo    = NULL;

    return (DoMethodA (object, (Msg)&ops));
    AROS_LIBFUNC_EXIT
} /* SetAttrsA */
