#include "async.h"


/* send out an async packet to the file system. */
VOID
AS_SendPacket( struct AsyncFile *file, APTR arg2 )
{
#ifdef ASIO_NOEXTERNALS
	struct ExecBase	*SysBase;

	SysBase = file->af_SysBase;
#endif

	file->af_Packet.sp_Pkt.dp_Port = &file->af_PacketPort;
	file->af_Packet.sp_Pkt.dp_Arg2 = ( SIPTR ) arg2;
	PutMsg( file->af_Handler, &file->af_Packet.sp_Msg );
	file->af_PacketPending = TRUE;
}
