#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, WriteAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buffer, A1),
        AROS_LHA(LONG, numBytes, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 10, Asyncio)

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

	LONG totalBytes = 0;

	/* this takes care of NIL: */
	if( !file->af_Handler )
	{
		file->af_Offset		= file->af_Buffers[ 0 ];
		file->af_BytesLeft	= file->af_BufferSize;
		return( numBytes );
	}

	while( numBytes > file->af_BytesLeft )
	{
		if( file->af_BytesLeft )
		{
			CopyMem( buffer, file->af_Offset, file->af_BytesLeft );

			numBytes	-= file->af_BytesLeft;
			buffer		=  ( APTR ) ( ( IPTR ) buffer + file->af_BytesLeft );
			totalBytes	+= file->af_BytesLeft;
		}

		if( AS_WaitPacket( file ) < 0 )
		{
			return( -1 );
		}

		/* send the current buffer out to disk */
		AS_SendPacket( file, file->af_Buffers[ file->af_CurrentBuf ] );

		file->af_CurrentBuf	= 1 - file->af_CurrentBuf;
		file->af_Offset		= file->af_Buffers[ file->af_CurrentBuf ];
		file->af_BytesLeft	= file->af_BufferSize;
	}

	CopyMem( buffer, file->af_Offset, numBytes );
	file->af_BytesLeft	-= numBytes;
	file->af_Offset		+= numBytes;

	return ( totalBytes + numBytes );

        AROS_LIBFUNC_EXIT
}
