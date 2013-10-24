/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the attributes of a BOOPSI object
    Lang: english
*/

#define AROS_TAGRETURNTYPE IPTR

#include <intuition/classes.h>
#include <intuition/intuitionbase.h>
#include "alib_intern.h"

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
#include <intuition/classusr.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	IPTR SetAttrs (

/*  SYNOPSIS */
	APTR  object,
	Tag   tag1,
	...)

/*  FUNCTION
	Changes several attributes of an object at the same time. How the
	object interprets the new attributes depends on the class.

    INPUTS
	object - Change the attributes of this object
	tag1 - The first of a list of attribute/value-pairs. The last
		attribute in this list must be TAG_END or TAG_DONE.
		The value for this last attribute is not examined (ie.
		you need not specify it).

    RESULT
	Depends on the class. For gadgets, this value is non-zero if
	they need redrawing after the values have changed. Other classes
	will define other return values.

    NOTES
	This function sends OM_SET to the object.

    EXAMPLE

    BUGS

    SEE ALSO
	intuition.library/NewObjectA(), intuition.library/DisposeObject(),
	intuition.library/GetAttr(), intuition.library/MakeClass(),
	"Basic Object-Oriented Programming System for Intuition" and
	"BOOPSI Class Reference" Document.

    INTERNALS

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetAttrsA (object, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetAttrs */
