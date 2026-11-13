#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, WriteCharAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(UBYTE, ch, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 12, Asyncio)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
        AROS_LIBFUNC_INIT

	if( file->af_BytesLeft )
	{
		/* if there's any room left in the current buffer, directly write
		 * the byte into it, updating counters and stuff.
		 */

		*file->af_Offset = ch;
		--file->af_BytesLeft;
		++file->af_Offset;

		/* one byte written */
		return( 1 );
	}

	/* there was no room in the current buffer, so call the main write
	 * routine. This will effectively send the current buffer out to disk,
	 * wait for the other buffer to come back, and then put the byte into
	 * it.
	 */

	{
		TEXT	c;

		c = ch;		/* SAS/C workaround... */

		return( WriteAsync( file, &c, 1 ) );
	}

        AROS_LIBFUNC_EXIT
}
