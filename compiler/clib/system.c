/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    ANSI C function system().
*/

#include <utility/tagitem.h>
#include <dos/dos.h>
#include <proto/dos.h>

/*****************************************************************************

    NAME */
#include <stdlib.h>

	int system (

/*  SYNOPSIS */
	const char *string)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
    static struct TagItem list[]={ { TAG_END,0 } }; /* No Tags */

    if (string == NULL)
    {
	return 1;
    }
    else
    {
	return (UBYTE)SystemTagList ((char *)string, list);
    }

} /* system */

