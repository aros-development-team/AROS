/*
    (C) 2000 AROS - The Amiga Research OS
    $Id$

    Desc: Varargs version of dos.library/AllocDosObject()
    Lang: english
*/
#define AROS_TAGRETURNTYPE APTR
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	APTR AllocDosObjectTags (

/*  SYNOPSIS */
	ULONG type,
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of dos.library/AllocDosObject().
        For information see dos.library/AllocDosObject().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos/AllocDosObject()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    AllocDosObject (type, AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* AllocDosObjectTags */
