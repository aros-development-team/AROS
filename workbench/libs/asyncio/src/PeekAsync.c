#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, PeekAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buffer, A1),
        AROS_LHA(LONG, numBytes, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 17, Asyncio)

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

	/* Try to fill a new buffer, if needed */
	if( !file->af_BytesLeft )
	{
		LONG	bytes;

		if( ( bytes = ReadAsync( file, &bytes, 1 ) ) <= 0 )
		{
			return( bytes );
		}

		/* Unread byte */
		--file->af_Offset;
		++file->af_BytesLeft;
	}

	/* Copy what we can */
	numBytes = MIN( numBytes, file->af_BytesLeft );
	CopyMem( file->af_Offset, buffer, numBytes );
	return( numBytes );

        AROS_LIBFUNC_EXIT
}
