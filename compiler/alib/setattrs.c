/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.2  1996/09/17 18:05:45  digulla
    Same names for same parameters

    Revision 1.1  1996/08/28 17:52:29  digulla
    First step to implement amiga.lib
    BOOPSI Utility functions


    Desc:
    Lang: english
*/
#include <intuition/classes.h>
#include <intuition/intuitionbase.h>
#include <stdarg.h>

extern struct IntuitionBase * IntuitionBase;

/*****************************************************************************

    NAME */
	#include <intuition/classusr.h>
	#include <clib/intuition_protos.h>

	ULONG SetAttrs (

/*  SYNOPSIS */
	APTR  object,
	ULONG tag1,
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
    va_list args;
    ULONG   retval;

    va_start (args, tag1);

    retval = SetAttrsA (object, (struct TagItem *)&tag1);

    va_end (args);

    return (retval);
} /* SetAttrs */
