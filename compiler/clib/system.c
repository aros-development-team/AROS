/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc: ANSI C function system()
    Lang: English
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

    HISTORY
	18.05.2001 Derived from libnix

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

