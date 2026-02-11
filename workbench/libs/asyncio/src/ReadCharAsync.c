#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH1(LONG, ReadCharAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 11, Asyncio)

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

	UBYTE	ch;

	if( file->af_BytesLeft )
	{
		/* if there is at least a byte left in the current buffer, get it
		 * directly. Also update all counters
		 */

		ch = *file->af_Offset;
		--file->af_BytesLeft;
		++file->af_Offset;

		return( ch );
	}

	/* there were no characters in the current buffer, so call the main read
	 * routine. This has the effect of sending a request to the file system to
	 * have the current buffer refilled. After that request is done, the
	 * character is extracted for the alternate buffer, which at that point
	 * becomes the "current" buffer
	 */

	if( ReadAsync( file, &ch, 1 ) > 0 )
	{
		return( ch );
	}

	/* We couldn't read above, so fail */

	return( -1 );

        AROS_LIBFUNC_EXIT
}
