/*
 * $VER: DetectPaulaAudio.c 1.0 (14.12.01) ® 2001 by Przemyslaw 'SENSEI' Gruchala <sensei@box43.gnet.pl>
 */

#include	<string.h>
 
#include	<devices/ahi.h>
#include	<dos/dos.h>
#include	<exec/devices.h>
#include	<exec/libraries.h>
#include	<exec/types.h>
#include	<utility/tagitem.h>
 
#include	<proto/ahi.h>
#include	<proto/dos.h>
#include	<proto/exec.h>

UBYTE	VersionID[]	= "$VER: DetectPaulaAudio.exe 1.0 (14.12.01) ® 2001 by Przemyslaw \'SENSEI\' Gruchala <sensei@box43.gnet.pl>";

int main( void )
{
	int	result	= RETURN_FAIL;
	struct MsgPort	*ahireplyport;
	if( ahireplyport = CreateMsgPort() )
	{
		struct AHIRequest	*ahirequest;
		if( ahirequest = CreateIORequest( ahireplyport, sizeof( *ahirequest ) ) )
		{
			ahirequest->ahir_Version	= 4;
			if( !OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest *) ahirequest, 0 ) )
			{
				struct Library	*AHIBase	= &ahirequest->ahir_Std.io_Device->dd_Library;
				UBYTE	drivername[ 256 ];
				if( AHI_GetAudioAttrs( AHI_DEFAULT_ID, NULL,
					AHIDB_Driver,		drivername,
					AHIDB_BufferLen,	sizeof( drivername ),
					TAG_DONE ) )
				{
					Printf( "Driver name \"%s\".\n", drivername );
					if( !stricmp( drivername, "paula" ) )
					{
						Printf( "Paula detected!\n" );
						result	= RETURN_OK;
					}
					else
					{
						Printf( "Paula is not detected!\n" );
						result	= RETURN_WARN;
					}
				}
				else
					Printf( "Could not get AHI driver name!\n" );
				CloseDevice( (struct IORequest *) ahirequest );
			}
			else
				Printf( "Could not open \"%s\" V%ld!\n", AHINAME, ahirequest->ahir_Version );
			DeleteIORequest( (struct IORequest *) ahirequest );
		}
		else
			Printf( "Could not create AHI request!\n" );
		DeleteMsgPort( ahireplyport );
	}
	else
		Printf( "Could not create AHI reply port!\n" );
	return( result );
}
 
