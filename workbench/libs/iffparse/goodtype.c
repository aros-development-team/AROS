/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 0
#include <aros/debug.h>
#include "iffparse_intern.h"

#define DEBUG_GOODTYPE(x)	;

/*****************************************************************************

    NAME */
#include <proto/iffparse.h>

	AROS_LH1(LONG, GoodType,

/*  SYNOPSIS */
	AROS_LHA(LONG, type, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 44, IFFParse)

/*  FUNCTION
	Determines whether a IFF chunk type is valid according to the IFF specification.

    INPUTS
	type  - An IFF chunk type to be tested.

    RESULT
	TRUE  - type is valid.
	FALSE  -  otherwise.

    NOTES
	Assumes the input type to be in local byte order.

    EXAMPLE

    BUGS

    SEE ALSO
	GoodID()

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    UBYTE theId[4];
    WORD  i;

   /* How can it be a valid type if its not a valid ID */
    if(!GoodID(type))
    {
	DEBUG_GOODTYPE(bug("badtype 1\n"));
	return (FALSE);
    }

    theId[0] = type >> 24;
    theId[1] = type >> 16;
    theId[2] = type >> 8;
    theId[3] = type;

    for(i=0; i < 4; i++)
    {
	/* Greater than Z, not a type */
	if(theId[i] > 'Z')
	{
	    DEBUG_GOODTYPE(bug("badtype 2\n"));
            return (FALSE);
	}

	/*  If its less than 'A', and not in '0'..'9',
	    then if its not a space its not valid. */
	if(    (theId[i] < 'A')
	    && ((theId[i] < '0') || (theId[i] > '9'))
	    && (theId[i] != ' ')
	  )
	{
	    DEBUG_GOODTYPE(bug("badtype 3\n"));
            return (FALSE);
	}

	/* Must be valid, try the next one */
    }

    DEBUG_GOODTYPE(bug("goodtype\n"));
    return (TRUE);

    AROS_LIBFUNC_EXIT
} /* GoodType */
