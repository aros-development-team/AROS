#include "async.h"


/* this function puts the packet back on the message list of our
 * message port.
 */
VOID
AS_RequeuePacket( AsyncFile *file )
{
#ifdef ASIO_NOEXTERNALS
	struct ExecBase	*SysBase = file->af_SysBase;
#endif

	AddHead( &file->af_PacketPort.mp_MsgList, &file->af_Packet.sp_Msg.mn_Node );
	file->af_PacketPending = TRUE;
}
