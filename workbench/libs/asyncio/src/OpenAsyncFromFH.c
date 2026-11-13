#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(AsyncFile *, OpenAsyncFromFH,

/*  SYNOPSIS */
        AROS_LHA(BPTR, handle, A0),
        AROS_LHA(OpenModes, mode, D0),
        AROS_LHA(LONG, bufferSize, D1),

/*  LOCATION */
        struct Library *, AsyncIOBase, 6, Asyncio)

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

	return( AS_OpenAsyncFH( handle, mode, bufferSize, FALSE ) );
        
        AROS_LIBFUNC_EXIT
}

