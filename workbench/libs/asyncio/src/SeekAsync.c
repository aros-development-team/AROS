#include "async.h"


static ULONG
GetFileSize( AsyncFile *file, LONG *size )
{
	D_S( struct FileInfoBlock, fib );

	if( !ExamineFH( file->af_File, fib ) )
	{
		AS_RecordSyncFailure( file );
		return( FALSE );
	}

	*size = fib->fib_Size;
	return( TRUE );
}


/*****************************************************************************

    NAME */
        AROS_LH3(LONG, SeekAsync,

/*  SYNOPSIS */
        AROS_LHA(AsyncFile *, file, A0),
        AROS_LHA(LONG, position, D0),
        AROS_LHA(SeekModes, mode, D1),

/*  LOCATION */
        struct Library *, AsyncIOBase, 8, Asyncio)

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

	LONG	current, target, roundTarget, filePos;
	LONG	minBuf, maxBuf, bytesArrived, diff;
	LONG	fileSize;

	bytesArrived = AS_WaitPacket( file );

	/* MH: No packets can be pending here! */

	if( bytesArrived < 0 )
	{
		/* MH: Experimental: Try to allow "resume" of seeks past EOF. */

		if( file->af_SeekPastEOF )
		{
			/* MH: Restore saved values, to make resume possible */
			bytesArrived = file->af_LastRes1;
			file->af_BytesLeft = file->af_LastBytesLeft;
		}
		else
		{
			return( -1 );
		}
	}

	if( file->af_ReadMode )
	{
		/* figure out what the actual file position is */
		filePos = Seek( file->af_File, 0, OFFSET_CURRENT );

		if( filePos < 0 )
		{
			AS_RecordSyncFailure( file );
			return( -1 );
		}

		/* figure out what the caller's file position is */
		current = filePos - ( file->af_BytesLeft + bytesArrived ) + file->af_SeekOffset;

		/* MH: We can't clear af_SeekOffset here. If another seek is done
		 * directly after this one, it would mean that we will both return
		 * the wrong position, and start reading from the wrong position.
		 */
		/* file->af_SeekOffset = 0; */

		/* figure out the absolute offset within the file where we must seek to */
		if( mode == MODE_CURRENT )
		{
			target = current + position;
		}
		else if( mode == MODE_START )
		{
			target = position;
		}
		else /* if( mode == MODE_END ) */
		{
			if( !GetFileSize( file, &fileSize ) )
			{
				return( -1 );
			}

			target = fileSize + position;
		}

		/* MH: Here we must be able to handle two different situations:
		 * 1) A seek directly after having dropped both buffers, and started
		 *    refilling (typical case: File open).
		 * 2) Other seeks (typical case: A seek after some initial reading).
		 *
		 * We need to subtract with "af_Buffers[ 1 - file->af_CurrentBuf ]",
		 * as af_CurrentBuf refers to the *arrived* buffer, not the one we're
		 * currently reading from (and af_Offset points into the buffer we're
		 * reading from)!
		 *
		 * In case 1, there will be only one packet received. af_CurrentBuf
		 * will be zero, and refers to the newly arrived buffer (as it
		 * should). For proper behaviour in the minBuf calculation, we have
		 * set af_Offset to point to af_Buffers[ 1 ], when starting reading
		 * to empty buffers. That way wee need no special case code here.
		 * ReadAsync() can handle this, as af_BytesLeft == 0 in that case.
		 */

		/* figure out what range of the file is currently in our buffers */
		minBuf = current - ( LONG ) ( file->af_Offset - file->af_Buffers[ 1 - file->af_CurrentBuf ] );
		maxBuf = current + file->af_BytesLeft + bytesArrived;  /* WARNING: this is one too big */

		diff = target - current;

#ifdef DO_SOME_DEBUG
		Printf( "Target: %ld, minBuf: %ld, maxBuf: %ld, current: %ld, diff: %ld, bytesLeft: %ld\n",
			target, minBuf, maxBuf, current, diff, file->af_BytesLeft );
#endif

		if( ( target < minBuf ) || ( target >= maxBuf ) )
		{
			/* the target seek location isn't currently in our buffers, so
			 * move the actual file pointer to the desired location, and then
			 * restart the async read thing...
			 */

			if( target >= maxBuf )
			{
				/* MH: There's a fair chance that we really tried to seek
				 * past EOF. In order to tell for sure, we need to compare
				 * the seek target with the file size. The roundTarget may
				 * be before real EOF, so the "real" Seek() call might
				 * not notice any problems.
				 */
				if( !GetFileSize( file, &fileSize ) )
				{
					return( -1 );
				}

				if( target > fileSize )
				{
					/* MH: Experimental: Try to allow "resume" of
					 * seeks past EOF.
					 */
					file->af_SeekPastEOF = TRUE;

					SetIoErr( ERROR_SEEK_ERROR );
					AS_RecordSyncFailure( file );
					return( -1 );
				}
			}

			/* this is to keep our file reading block-aligned on the device.
			 * block-aligned reads are generally quite a bit faster, so it is
			 * worth the trouble to keep things aligned
			 */
			roundTarget = ( target / file->af_BlockSize ) * file->af_BlockSize;

			if( Seek( file->af_File, roundTarget - filePos, OFFSET_CURRENT ) < 0 )
			{
				AS_RecordSyncFailure( file );
				return( -1 );
			}

			AS_SendPacket( file, file->af_Buffers[ 0 ] );

			file->af_SeekOffset	= target - roundTarget;
			file->af_BytesLeft	= 0;
			file->af_CurrentBuf	= 0;

			/* MH: We set af_Offset to the buffer not being filled, to be able to
			 * handle a new seek directly after this one (see above; minBuf
			 * calculation). If we start reading after this seek, ReadAsync()
			 * will handle everything correctly, as af_BytesLeft == 0.
			 */
			file->af_Offset		= file->af_Buffers[ 1 ];
		}
		else if( ( target < current ) || ( diff <= file->af_BytesLeft ) )
		{
			/* one of the two following things is true:
			 *
			 * 1. The target seek location is within the current read buffer,
			 * but before the current location within the buffer. Move back
			 * within the buffer and pretend we never got the pending packet,
			 * just to make life easier, and faster, in the read routine.
			 *
			 * 2. The target seek location is ahead within the current
			 * read buffer. Advance to that location. As above, pretend to
			 * have never received the pending packet.
			 */

			AS_RequeuePacket( file );

			file->af_BytesLeft	-= diff;
			file->af_Offset		+= diff;

			/* MH: We don't need to clear the seek offset here, since
			 * if we get here, we must have read some data from the current
			 * buffer, and af_SeekOffset will be zero then (done by
			 * ReadAsync()).
			 */

			/* MH: If we're recovering from seek past EOF, restore some
			 * values here.
			 */
			if( file->af_SeekPastEOF )
			{
				file->af_Packet.sp_Pkt.dp_Res1 = file->af_LastRes1;
			}
		}
		else
		{
			/* at this point, we know the target seek location is within
			 * the buffer filled in by the packet that we just received
			 * at the start of this function. Throw away all the bytes in the
			 * current buffer, send a packet out to get the async thing going
			 * again, readjust buffer pointers to the seek location, and return
			 * with a grin on your face... :-)
			 */

			/* MH: Don't refill the buffer we just got, but the other one! */
			AS_SendPacket( file, file->af_Buffers[ 1 - file->af_CurrentBuf ] );

			/* MH: Account for bytes left in buffer we drop *and* the af_SeekOffset.
			 */
			diff -= file->af_BytesLeft - file->af_SeekOffset;

			/* MH: Set the offset into the current (newly arrived) buffer */
			file->af_Offset = file->af_Buffers[ file->af_CurrentBuf ] + diff;
			file->af_BytesLeft = bytesArrived - diff;

			/* MH: We need to clear the seek offset here, since we can't do it above.
			 */
			file->af_SeekOffset = 0;

			/* MH: This "buffer switching" is important to do. It wasn't done!
			 * This explains the errors one could encounter now and then.
			 * The AS_SendPacket() call above is not the cause, and *is* correct.
			 */
			file->af_CurrentBuf = 1 - file->af_CurrentBuf;
		}
	}
	else
	{
		/* flush the buffers */
		if( file->af_BufferSize > file->af_BytesLeft )
		{
			if( Write(
				file->af_File,
				file->af_Buffers[ file->af_CurrentBuf ],
				file->af_BufferSize - file->af_BytesLeft ) < 0 )
			{
				AS_RecordSyncFailure( file );
				return( -1 );
			}
		}

		/* this will unfortunately generally result in non block-aligned file
		 * access. We could be sneaky and try to resync our file pos at a
		 * later time, but we won't bother. Seeking in write-only files is
		 * relatively rare (except when writing IFF files with unknown chunk
		 * sizes, where the chunk size has to be written after the chunk data)
		 */

		/* MH: Ideas on how to improve the above (not tested, since I don't need
		 * the SeekAsync for writing in any of my programs at the moment! ;):
		 *
		 * Add a new field to the AsyncFile struct. af_WriteOffset or something like
		 * that (af_SeekOffset can probably be used). Like in the read case, we
		 * calculate a roundTarget, but we don't seek to that (but rather to the
		 * "absolute" position), and save the difference in the struct. af_BytesLeft
		 * and af_Offset are adjusted to point into the "middle" of the buffer,
		 * where the write will occur. Writes then needs some minor changes:
		 * Instead of simply writing the buffer from the start, we add the offset
		 * (saved above) to the buffer base, and write the partial buffer. The
		 * offset is then cleared. Voila: The file is still block-aligned, at the
		 * price of some non-optimal buffer usage.
		 *
		 * Problem: As it is now, Arg3 in the packet is always set to the buffer size.
		 * With the above fix, this would have to be updated for each SendPacket (i.e.
		 * a new argument would be needed).
		 */

		current = Seek( file->af_File, position, mode );

		if( current < 0 )
		{
			AS_RecordSyncFailure( file );
			return( -1 );
		}

		file->af_BytesLeft	= file->af_BufferSize;
		file->af_CurrentBuf	= 0;
		file->af_Offset		= file->af_Buffers[ 0 ];
	}

	if( file->af_SeekPastEOF )
	{
		/* MH: Clear up any error flags, and restore last Res1. */
		file->af_SeekPastEOF = FALSE;
	}

	SetIoErr( 0 );
	return( current );

        AROS_LIBFUNC_EXIT
}
