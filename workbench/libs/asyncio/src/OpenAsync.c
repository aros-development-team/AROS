#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(AsyncFile *, OpenAsync,

/*  SYNOPSIS */
        AROS_LHA(const STRPTR, fileName, A0),
        AROS_LHA(OpenModes, mode, D0),
        AROS_LHA(LONG, bufferSize, D1),

/*  LOCATION */
        struct Library *, AsyncIOBase, 5, Asyncio)

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

	static const WORD PrivateOpenModes[] =
	{
		MODE_OLDFILE, MODE_NEWFILE, MODE_READWRITE
	};
	BPTR		handle;
	AsyncFile	*file = NULL;

	if( handle = Open( fileName, PrivateOpenModes[ mode ] ) )
	{
		file = AS_OpenAsyncFH( handle, mode, bufferSize, TRUE );

		if( !file )
		{
			Close( handle );
		}
	}

	return( file );

        AROS_LIBFUNC_EXIT
}
