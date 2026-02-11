#include "async.h"


/* this function records a failure from a synchronous DOS call into the
 * packet so that it gets picked up by the other IO routines in this module
 */
VOID
AS_RecordSyncFailure( AsyncFile *file )
{
#ifdef ASIO_NOEXTERNALS
	struct DosLibrary	*DOSBase = file->af_DOSBase;
#endif

	/* MH: Back up some values to make it possible to resume operation
	 * after seeks past EOF.
	 */
	file->af_LastRes1 = file->af_Packet.sp_Pkt.dp_Res1;
	file->af_LastBytesLeft = file->af_BytesLeft;

	file->af_Packet.sp_Pkt.dp_Res1	= -1;
	file->af_Packet.sp_Pkt.dp_Res2	= IoErr();
	/* MH: To make sure we can't read/write despite an error condition */
	file->af_BytesLeft = 0;
}
