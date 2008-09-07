/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Allocate a NamedObject with varargs.
    Lang: english
*/
#define AROS_TAGRETURNTYPE  struct NamedObject *

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/utility.h>
#include "alib_intern.h"

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <utility/name.h>
	#include <proto/utility.h>

	struct NamedObject * AllocNamedObject (

/*  SYNOPSIS */
	STRPTR             name,
	Tag                tag1,
	...                )

/*  FUNCTION
	Allocate a NamedObject. This is the varargs version of the function.
	For more information see AllocNamedObjectA() in utility.library.

    INPUTS
	name        -   The name of the object to allocate.
	tag1        -   The first Tag of the arguments. End the list with
			TAG_DONE.

    RESULT
	The address of a NamedObject, or NULL if the allocation failed.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	utility.library/AllocNamedObjectA()

    INTERNALS

    HISTORY
	09-02-1997  iaint       Adapted from another function.

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)

    retval = AllocNamedObjectA(name, AROS_SLOWSTACKTAGS_ARG(tag1));

    AROS_SLOWSTACKTAGS_POST
} /* AllocNamedObject */
