/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <dos/dosextens.h>
#include <proto/icon.h>

extern struct Library *IconBase;

       STRPTR   ArgString(

/*  SYNOPSIS */
       UBYTE  **tt,
       STRPTR   entry,
       STRPTR   defaultstring)
            

/*  FUNCTION
	Looks for 'entry' in the tooltypes array 'tt'. If not found
	'defaultstring' is returned, otherwise the string corresponding to
	'entry' is returned. Say the tooltype Settings=SaveonExit exists
	within the array, then a pointer to "SaveonExit" is returned when
	ArgString(tt, "Settings") is done.

    INPUTS
	tt     --  the tooltype array to search in
	entry  --  the string to look for in the tooltype array

    RESULT
	Pointer to the string corresponding to 'entry' in the tooltype
	array, or 'defaultstring' if 'entry' was not found.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ArgArrayInit()

    INTERNALS

    HISTORY
	28.04.98  SDuvan  implemented

*****************************************************************************/
{
    STRPTR  match;

    if((match = FindToolType(tt, entry)) == NULL)
	return defaultstring;

    return match;
    
} /* ArgArrayInt */

