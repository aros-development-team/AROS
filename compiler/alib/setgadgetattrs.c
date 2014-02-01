/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Change the attributes of a BOOPSI gadget
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
#include <intuition/intuition.h>
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/intuition.h>

	IPTR SetGadgetAttrs (

/*  SYNOPSIS */
	struct Gadget    * Gadget,
	struct Window    * Window,
	struct Requester * Requester,
	Tag                tag1,
	...)

/*  FUNCTION
	Changes several attributes of a gadget at the same time. How the
	gadget interprets the new attributes depends on the class.

    INPUTS
	Gadget - Change the attributes of this object
	Window - The window the gadget is in
	Requester - The Requester the gadget is in, may be NULL
	tag1 - The first of a list of attribute/value-pairs. The last
		attribute in this list must be TAG_END or TAG_DONE.
		The value for this last attribute is not examined (ie.
		you need not specify it).

    RESULT
	This value is non-zero if the gadget needs redrawing after the
	values have changed.

    NOTES
	This function sends OM_SET to the object.

    EXAMPLE

    BUGS

    SEE ALSO
	NewObject(), SetAttrs(),
	"Basic Object-Oriented Programming System for Intuition" and
	"boopsi Class Reference" documentation.

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = SetGadgetAttrsA (Gadget, Window, Requester, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* SetGadgetAttrs */
