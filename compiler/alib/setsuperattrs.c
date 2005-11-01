/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$

    Set attributes in a specific class
*/

#define INTUITION_NO_INLINE_STDARG

#define AROS_TAGRETURNTYPE IPTR

#include <intuition/classes.h>
#include <intuition/intuitionbase.h>
#include "alib_intern.h"

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#include <proto/intuition.h>

	IPTR SetSuperAttrs (

/*  SYNOPSIS */
	Class *  class,
	Object * object,
	Tag	 tag1,
	...)

/*  FUNCTION
	Changes several attributes of an object at the same time. How the
	object interprets the new attributes depends on the class.

    INPUTS
	class - Assume that the object is of this class.
	object - Change the attributes of this object
	tag1 - The first of a list of attribute/value-pairs. The last
		attribute in this list must be TAG_END or TAG_DONE.
		The value for this last attribute is not examined (ie.
		you need not specify it).

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
    struct opSet ops;

    ops.MethodID     = OM_SET;
    ops.ops_AttrList = &tag1;
    ops.ops_GInfo    = NULL;

    return DoSuperMethodA(class, object, (Msg)&ops);
} /* SetSuperAttrs */
