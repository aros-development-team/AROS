#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH4(APTR, FGetsLenAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buf, A1),
        AROS_LHA(LONG, numBytes, D0),
        AROS_LHA(LONG *, len, A2),

/*  LOCATION */
        struct Library *, AsyncIOBase, 16, Asyncio)

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

	UBYTE	*p;
	LONG	length = 0;

	p = ( UBYTE * ) buf;

	/* Make room for \n and \0 */
	if( --numBytes <= 0 )
	{
		/* Handle senseless cases */
		return( NULL );
	}

	while( TRUE )
	{
		UBYTE	*ptr;
		LONG	i, count;

		ptr = ( UBYTE * ) file->af_Offset;

		if( count = file->af_BytesLeft )
		{
			count = MIN( count, numBytes );

			for( i = 0; ( i < count ) && ( *ptr != '\n' ); ++i )
			{
				*p++ = *ptr++;
			}

			length += i;

			/* Check for valid EOL char */
			if( i < count )
			{
				/* MH: Since i < count, and count <= numBytes,
				 * there _is_ room for \n\0.
				 */
				*p++ = '\n';	/* "Read" EOL char */
				++i;
				length += 1;
			}

			file->af_BytesLeft -= i;
			file->af_Offset    += i;

			if( ( i >= numBytes ) || ( *( p - 1 ) == '\n' ) )
			{
				/* MH: It is enough to break out of the loop.
				 * no need to "waste" code by making a special
				 * exit here. ;)
				 */
				break;
			}

			numBytes -= i;
		}

		/* MH: numBytes must be at least 1 here, so there is still room
		 * for \n\0, in case we read \n.
		 */

		if( ReadAsync( file, p, 1 ) < 1 )
		{
			break;
		}

		--numBytes;
		++length;

		if( *p++ == '\n' )
		{
			break;
		}
	}

	*p = '\0';
	*len = length;

	if( p == ( UBYTE * ) buf )
	{
		return( NULL );
	}

	return( buf );

        AROS_LIBFUNC_EXIT
}


/*****************************************************************************

    NAME */
        AROS_LH3(APTR, FGetsAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buf, A1),
        AROS_LHA(LONG, numBytes, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 15, Asyncio)

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

	return( FGetsLenAsync( file, buf, numBytes, &len ) );

        AROS_LIBFUNC_EXIT
}
