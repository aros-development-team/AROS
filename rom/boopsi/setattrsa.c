/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/
#include <proto/alib.h>
#include <clib/intuition_protos.h>
#include "intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/boopsi.h>

	AROS_LH2(ULONG, SetAttrsA,

/*  SYNOPSIS */
	AROS_LHA(APTR            , object, A0),
	AROS_LHA(struct TagItem *, tagList, A1),

/*  LOCATION */
	struct Library *, BOOPSIBase, 14, BOOPSI)

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
	NewObjectA(), DisposeObject(), GetAttr(), MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" Dokument.

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    intuition_lib.fd and clib/intuition_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    struct opSet ops;

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = tagList;
    ops.ops_GInfo    = NULL;

    return (DoMethodA (object, (Msg)&ops));
    AROS_LIBFUNC_EXIT
} /* SetAttrsA */
