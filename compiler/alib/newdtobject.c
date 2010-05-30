/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of datatypes.library/NewDTObject()
    Lang: english
*/

#define AROS_TAGRETURNTYPE Object *
#include <intuition/classusr.h>
#include <utility/tagitem.h>

#include <aros/debug.h>

extern struct Library *DataTypesBase;

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/datatypes.h>

	Object * NewDTObject (

/*  SYNOPSIS */
	APTR name,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of datatypes.library/NewDTObjectA().
        For information see datatypes.library/NewDTObjectA().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	datatypes.library/NewDTObjectA()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = NewDTObjectA (name, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
    
    kprintf("NewDTObject: leave (alib)\n");
} /* NewDTObject */
