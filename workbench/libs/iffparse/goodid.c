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

	AROS_LH1(LONG, GoodID,

/*  SYNOPSIS */
	AROS_LHA(LONG, id, D0),

/*  LOCATION */
	struct Library *, IFFParseBase, 43, IFFParse)

/*  FUNCTION
	Determines whether an ID is valid according to the IFF specification.

    INPUTS
	id - An IFF chunk ID to be tested.

    RESULT
	TRUE if valid.
	FALSE otherwise.

    NOTES
	Assumes input to be in local byte order.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
  27-11-96    digulla automatically created from
	  iffparse_lib.fd and clib/iffparse_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,IFFParseBase)

    UBYTE *theId;

  /* Assure big endian before checking */
    id = SwitchIfLittleEndian(id);

    theId = (UBYTE *)&id;


    /* If the ID starts with a space, but is not all spaces, then invalid */
    if((theId[0] == 0x20) && (id != 0x20202020))
	return (FALSE);

    /*
	Check whether the ID is within the allowed character ranges.
	This loop is unrolled
    */

    if( (theId[0] < 0x20) || (theId[1] > 0x7e))
	return (FALSE);
    if( (theId[1] < 0x20) || (theId[1] > 0x7e))
	return (FALSE);
    if( (theId[2] < 0x20) || (theId[2] > 0x7e))
	return (FALSE);
    if( (theId[3] < 0x20) || (theId[3] > 0x7e))
	return (FALSE);

    return (TRUE);

    AROS_LIBFUNC_EXIT
} /* GoodID */
