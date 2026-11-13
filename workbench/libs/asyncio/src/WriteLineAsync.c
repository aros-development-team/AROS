#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH2(LONG, WriteLineAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(STRPTR, line, A1),

/*  LOCATION */
        struct Library *, AsyncIOBase, 14, Asyncio)

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

	/* Since SAS/C have an inlined strlen... */
#if defined( NOEXTERNALS ) && !defined( __SAS )
	LONG	i = 0;
	STRPTR	s = line;

	while( *s )
	{
		++i, ++s;
	}

	return( WriteAsync( file, line, i ) );
#else
	return( WriteAsync( file, line, strlen( line ) ) );
#endif

        AROS_LIBFUNC_EXIT
}
