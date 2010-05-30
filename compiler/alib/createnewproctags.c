/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Varargs version of dos.library/CreateNewProc()
    Lang: english
*/

#define AROS_TAGRETURNTYPE struct Process *
#include <dos/dosextens.h>
#include <utility/tagitem.h>

/*****************************************************************************

    NAME */
#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/dos.h>

	struct Process * CreateNewProcTags (

/*  SYNOPSIS */
	Tag tag1,
	...)

/*  FUNCTION
        This is the varargs version of dos.library/CreateNewProc().
        For information see dos.library/CreateNewProc().

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        dos.library/CreateNewProc()

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_SLOWSTACKTAGS_PRE(tag1)
    retval = CreateNewProc (AROS_SLOWSTACKTAGS_ARG(tag1));
    AROS_SLOWSTACKTAGS_POST
} /* CreateNewProcTags */
