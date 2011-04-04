#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <proto/exec.h>

#include "internal/debug.h"
#include "functions.h"
#include "giggledisk.h"
#include "header.h"
#include "readargs.h"
#include "requester.h"

/*
** Device
*/

/****************************************************************************/

/* /// Device_Open()
**
*/

/*****************************************************************************/

BOOL Device_Open( struct MDevice *mdevice, char *name, ULONG unit, ULONG flags, ULONG iosize )
{

	if( ( mdevice->MessagePort = CreateMsgPort() ) ) {
		if( ( mdevice->IORequest = CreateIORequest( mdevice->MessagePort, iosize ) ) ) {
			if( !OpenDevice(name, unit, mdevice->IORequest, flags ) ) {
	            mdevice->IORequest->io_Message.mn_Node.ln_Type = NT_REPLYMSG;
	            mdevice->DeviceOpen = TRUE;
	        }
	    }
	}
	return( mdevice->DeviceOpen );
}
/* \\\ */
/* /// Device_Close()
**
*/

/*****************************************************************************/

void Device_Close( struct MDevice *mdevice )
{

	if( mdevice->DeviceOpen ) {

		while( !CheckIO( mdevice->IORequest ) ) {
			AbortIO( mdevice->IORequest );

            WaitIO( mdevice->IORequest );  /* wait for outstanding timer */

        }
		CloseDevice( mdevice->IORequest );
        mdevice->DeviceOpen = 0;
    }
	if( mdevice->IORequest ) {
		DeleteIORequest( mdevice->IORequest );
        mdevice->IORequest = NULL;
    }

	if( mdevice->MessagePort ) {
		DeleteMsgPort( mdevice->MessagePort );
        mdevice->MessagePort = NULL;
    }

}
/* \\\ */


/* /// GD_OpenDevice() */

/*************************************************************************/

ULONG GD_OpenDevice( void )
{
ULONG result;
struct NSDeviceQueryResult nsdquery;
UWORD *cmdcheck;
UWORD cmdcount;

    result = MSG_ERROR_UnableToOpenDevice;
    requester_args[0] = readargs_array[ARG_DEVICE];
    requester_args[1] = readargs_array[ARG_UNIT];

    if( Device_Open( &device, (STRPTR) readargs_array[ARG_DEVICE], readargs_array[ARG_UNIT], 0L,sizeof( struct IOExtTD))) {
        debug("Device is open\n");

        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Data = &nsdquery;
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Length = sizeof( nsdquery );
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Offset = 0;
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Flags |= IOF_QUICK;

        result = 0;
        if( !DoIO( device.IORequest )) {

            if( (((struct IOExtTD *) device.IORequest)->iotd_Req.io_Actual >= 16) &&
                (((struct IOExtTD *) device.IORequest)->iotd_Req.io_Actual <= sizeof( nsdquery ) )) {
                debug("Device is NSD\n");
                device64 = FALSE;

                result = MSG_ERROR_DeviceIsNotTrackDiskCompatible;

                if( (nsdquery.DeviceType == NSDEVTYPE_TRACKDISK) ) {
                    result = 0;
                    cmdcount = 0;

                    for(cmdcheck = nsdquery.SupportedCommands; *cmdcheck; cmdcheck++) {
                        switch( *cmdcheck ) {
                            case NSCMD_TD_READ64:
                            case NSCMD_TD_WRITE64:
                                cmdcount++;
                                break;
                        }
                    }
                    if( cmdcount >= 2 ) {
						device64 = TRUE;
                        debug("NSD enabled\n");
                    }

                }

            }
        }
    }
	return( result);
}
/* \\\ */

/* /// PE_GetGeometry()
**
*/

/*************************************************************************/

ULONG PE_GetGeometry( struct PartitionEntry *pe )
{
struct DriveGeometry dg;

    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Command = TD_GETGEOMETRY;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Data = (APTR) &dg;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Length = sizeof( struct DriveGeometry );

    if( !DoIO( device.IORequest )) {
/* first the standard stuff */
        pe->Device                 = readargs_array[ARG_DEVICE];
        pe->Unit                   = readargs_array[ARG_UNIT];
        pe->StackSize              = 16384;
        pe->ENV.de_NumBuffers      = 50;
        pe->ENV.de_MaxTransfer     = readargs_array[ARG_MAXTRANSFER];
        pe->ENV.de_Mask            = 0x7ffffffe;
        pe->ENV.de_BootPri         = 10;
/* and now the geometry stuff */

        pe->ENV.de_SizeBlock      = dg.dg_SectorSize/4;
        pe->ENV.de_LowCyl         = 0L;
        pe->ENV.de_HighCyl        = dg.dg_Cylinders-1;
        pe->ENV.de_Surfaces       = dg.dg_Heads;
        pe->ENV.de_BlocksPerTrack = dg.dg_TrackSectors;
        pe->ENV.de_SectorPerBlock = dg.dg_CylSectors;
        pe->ENV.de_BufMemType     = dg.dg_BufMemType;

		debug( APPLICATIONNAME ": SizeBlock = %ld\n", pe->ENV.de_SizeBlock *4);
		debug( APPLICATIONNAME ": LowCyl = %ld\n", pe->ENV.de_LowCyl);
		debug( APPLICATIONNAME ": HighCyl = %ld\n", pe->ENV.de_HighCyl);
		debug( APPLICATIONNAME ": Surfaces = %ld\n", pe->ENV.de_Surfaces);
		debug( APPLICATIONNAME ": BlocksPerTrack = %ld\n", pe->ENV.de_BlocksPerTrack);
		debug( APPLICATIONNAME ": SectorsPerBlock = %ld\n", pe->ENV.de_SectorPerBlock);
		debug( APPLICATIONNAME ": BufMemType = %ld\n", pe->ENV.de_BufMemType);

    }
    return( 0L);
}
/* \\\ */
