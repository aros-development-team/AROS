#include "async.h"

/*****************************************************************************

    NAME */
        AROS_LH3(LONG, ReadAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(APTR, buffer, A1),
        AROS_LHA(LONG, numBytes, D0),

/*  LOCATION */
        struct Library *, AsyncIOBase, 9, Asyncio)

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
	LONG bytesArrived;

	/* if we need more bytes than there are in the current buffer, enter the
	 * read loop
	 */

	while( numBytes > file->af_BytesLeft )
	{
		/* drain buffer */
		CopyMem( file->af_Offset, buffer, file->af_BytesLeft );

		numBytes		-= file->af_BytesLeft;
		buffer			=  ( APTR ) ( ( IPTR ) buffer + file->af_BytesLeft );
		totalBytes		+= file->af_BytesLeft;
		file->af_BytesLeft	=  0;

		bytesArrived = AS_WaitPacket( file );

		if( bytesArrived <= 0 )
		{
			if( bytesArrived == 0 )
			{
				return( totalBytes );
			}

			return( -1 );
		}

		/* ask that the buffer be filled */
		AS_SendPacket( file, file->af_Buffers[ 1 - file->af_CurrentBuf ] );

		/* in case we tried to seek past EOF */
		if( file->af_SeekOffset > bytesArrived )
		{
			file->af_SeekOffset = bytesArrived;
		}

		file->af_Offset		= file->af_Buffers[ file->af_CurrentBuf ] + file->af_SeekOffset;
		file->af_CurrentBuf	= 1 - file->af_CurrentBuf;
		file->af_BytesLeft	= bytesArrived - file->af_SeekOffset;
		file->af_SeekOffset	= 0;
	}

	CopyMem( file->af_Offset, buffer, numBytes );
	file->af_BytesLeft	-= numBytes;
	file->af_Offset		+= numBytes;

	return( totalBytes + numBytes );

        AROS_LIBFUNC_EXIT
}
