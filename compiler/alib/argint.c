/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

/*****************************************************************************

    NAME */

#include <dos/dosextens.h>
#include <proto/icon.h>
#include <stdlib.h>

extern struct Library *IconBase;

       LONG  ArgInt(

/*  SYNOPSIS */
       UBYTE  **tt,
       STRPTR   entry,
       LONG     defaultval)
            

/*  FUNCTION
	Returns the value associated with the string 'entry' found in the
	tooltypes array 'tt'. If no match with entry was found,
	'defaultval' is returned.

    INPUTS
	tt          --  the tooltypes array ( returned by ArgArrayInit() )
	entry       --  entry to look for (in tooltype "entry=value")
	defaultval  --  value returned if 'entry' was not found

    RESULT
	(The tooltypes looks like "Entry=Value".) Returns Value if Entry
	was found; otherwise returns 'defaultval'.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	ArgArrayInit()

    INTERNALS
	The Amiga documentation says "requires that dos.library V36 or
	higher is opened". I can't see why.

    HISTORY
	29.04.98  SDuvan  implemented

*****************************************************************************/
{
    STRPTR  match;

    if((match = FindToolType(tt, entry)) == NULL)
	return defaultval;

    return atoi(match);
    
} /* ArgArrayInt */
