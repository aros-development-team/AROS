/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$
    $Log$
    Revision 1.3  1996/11/25 10:53:18  aros
    Allow stacktags on special CPUs

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
#include "alib_intern.h"

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
#ifdef AROS_SLOWSTACKTAGS
    ULONG	     retval;
    va_list	     args;
    struct TagItem * tags;

    va_start (args, tag1);

    if ((tags = GetTagsFromStack (tag1, args)))
    {
	retval = SetAttrsA (object, tags);

	FreeTagsFromStack (tags);
    }
    else
	retval = 0L; /* fail :-/ */

    va_end (args);

    return retval;
#else
    return SetAttrsA (object, (struct TagItem *)&tag1);
#endif
} /* SetAttrs */
