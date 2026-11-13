#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <proto/asyncio.h>
#include <proto/exec.h>
#include <proto/dos.h>


#include <string.h>


/*****************************************************************************/


/* this macro lets us long-align structures on the stack */
#define D_S(type,name) char a_##name[ sizeof( type ) + 3 ]; \
			type *name = ( type * ) ( ( SIPTR ) ( a_##name + 3 ) & ~3 );

#ifndef MIN
#define MIN(a,b) ( ( a ) < ( b ) ? ( a ) : ( b ) )
#endif


/*****************************************************************************/


AsyncFile *AS_OpenAsyncFH( BPTR handle, OpenModes mode, LONG bufferSize, BOOL closeIt );
VOID AS_SendPacket( AsyncFile *file, APTR arg2 );
LONG AS_WaitPacket( AsyncFile *file );
VOID AS_RequeuePacket( AsyncFile *file );
VOID AS_RecordSyncFailure( AsyncFile *file );

