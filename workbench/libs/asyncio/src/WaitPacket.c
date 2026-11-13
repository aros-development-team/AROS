#include "async.h"


/* this function waits for a packet to come back from the file system. If no
 * packet is pending, state from the previous packet is returned. This ensures
 * that once an error occurs, it state is maintained for the rest of the life
 * of the file handle.
 *
 * This function also deals with IO errors, bringing up the needed DOS
 * requesters to let the user retry an operation or cancel it.
 */
LONG
AS_WaitPacket( AsyncFile *file )
{
	LONG bytes;

	if( file->af_PacketPending )
	{
		while( TRUE )
		{
			/* This enables signalling when a packet comes back to the port */
			file->af_PacketPort.mp_Flags = PA_SIGNAL;

			/* Wait for the packet to come back, and remove it from the message
			 * list. Since we know no other packets can come in to the port, we can
			 * safely use Remove() instead of GetMsg(). If other packets could come in,
			 * we would have to use GetMsg(), which correctly arbitrates access in such
			 * a case
			 */
			Remove( ( struct Node * ) WaitPort( &file->af_PacketPort ) );

			/* set the port type back to PA_IGNORE so we won't be bothered with
			 * spurious signals
			 */
			file->af_PacketPort.mp_Flags = PA_IGNORE;

			/* mark packet as no longer pending since we removed it */
			file->af_PacketPending = FALSE;

			bytes = file->af_Packet.sp_Pkt.dp_Res1;

			if( bytes >= 0 )
			{
				/* packet didn't report an error, so bye... */
				return( bytes );
			}

			/* see if the user wants to try again... */
			if( ErrorReport( file->af_Packet.sp_Pkt.dp_Res2, REPORT_STREAM, (IPTR)file->af_File, NULL ) )
			{
				return( -1 );
			}

			/* user wants to try again, resend the packet */
			AS_SendPacket( file,
				file->af_Buffers[ file->af_ReadMode ?
					file->af_CurrentBuf :
					1 - file->af_CurrentBuf ] );
		}
	}

	/* last packet's error code, or 0 if packet was never sent */
	SetIoErr( file->af_Packet.sp_Pkt.dp_Res2 );

	return( file->af_Packet.sp_Pkt.dp_Res1 );
}
