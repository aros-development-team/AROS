#include "async.h"


/*****************************************************************************

    NAME */
        AROS_LH1(LONG, CloseAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 7, Asyncio)

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

	LONG	result;

	if( file )
	{
		result = AS_WaitPacket( file );

		if( result >= 0 )
		{
			if( !file->af_ReadMode )
			{
				/* this will flush out any pending data in the write buffer */
				if( file->af_BufferSize > file->af_BytesLeft )
				{
					result = Write(
						file->af_File,
						file->af_Buffers[ file->af_CurrentBuf ],
						file->af_BufferSize - file->af_BytesLeft );
				}
			}
		}

		if( file->af_CloseFH )
		{
			Close( file->af_File );
		}

		FreeVec(file);
	}
	else
	{
		SetIoErr( ERROR_INVALID_LOCK );
		result = -1;
	}

	return( result );

        AROS_LIBFUNC_EXIT
}
