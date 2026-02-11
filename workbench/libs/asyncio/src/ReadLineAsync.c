#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, ReadLineAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buffer, A1),
        AROS_LHA(LONG, size, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 13, Asyncio)

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

	LONG	len;

	/* First read any data up to the LF or the buffer is full */
	if( FGetsLenAsync( file, buffer, size, &len ) )
	{
		UBYTE	*end;

		end = ( ( UBYTE * ) buffer ) + len - 1;

		if( *end != '\n' )
		{
			UBYTE	ch = 0;

			/* We didn't reach EOF yet */
			while( TRUE )
			{
				UBYTE	*ptr;
				LONG	i, count;

				ptr = ( UBYTE * ) file->af_Offset;

				if( count = file->af_BytesLeft )
				{
					/* Scan for LF char in buffer */
					for( i = 0; ( i < count ) && ( *ptr != '\n' ); ++i, ++ptr )
					{
					}

					/* If i < count, then the loop above aborted
					 * due to LF char.
					 */
					if( i < count )
					{
						ch = '\n';
						++i;
					}

					file->af_BytesLeft -= i;
					file->af_Offset    += i;

					if( i < count )
					{
						/* All done */
						break;
					}
				}

				if( ReadAsync( file, &ch, 1 ) < 1 )
				{
					break;
				}

				if( ch == '\n' )
				{
					/* All done */
					break;
				}
			}

			if( ch == '\n' )
			{
				/* Overwrite last char with LF */
				*end++ = '\n';
				*end = '\0';
			}
		}
	}

	return( len );

        AROS_LIBFUNC_EXIT
}
