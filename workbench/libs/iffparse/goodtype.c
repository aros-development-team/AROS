/*
    (C) 1995-96 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include "iffparse_intern.h"

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

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    UBYTE *theId = (UBYTE *)&type;
    BYTE i;

   /* How can it be a valid type if its not a valid ID */
    if(!GoodID(type))
	return (FALSE);

    /* Assure Big endianess */
    type = SwitchIfLittleEndian(type);


    for(i=0; i < 4; i++, theId++)
    {
	/* Greater than Z, not a type */
	if(*theId > 'Z')
	    return (FALSE);

	/*  If its less than 'A', and not in '0'..'9',
	    then if its not a space its not valid. */
	if(    (*theId < 'A')
	    && ((*theId < '0') || (*theId > '9'))
	    && (*theId != ' ')
	  )
	    return (FALSE);

	/* Must be valid, try the next one */
    }
    return (TRUE);

    AROS_LIBFUNC_EXIT
} /* GoodType */
