
/*
** giggledisk.c
**
** (c) 1998 - 2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "giggledisk.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <exec/types.h>
#include <dos/dos.h>
#include <workbench/workbench.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>

#include "create.h"
#include "createpartitionlist.h"
#include "functions.h"
#include "giggledisk.h"
#include "header.h"
#include "macros.h"
#include "main.h"
#include "mount.h"
#include "readargs.h"
#include "requester.h"
#include "version.h"

/*************************************************************************/

/* /// MD_ShowPartitions()
**
*/

/*************************************************************************/

static void MD_ShowPartitions( void )
{
struct PartitionEntry *pe;
ULONG args[15], i;
ULONG *arg;

	VPrintf("\nGiggleDisk © 2005 - 2011 Guido Mersmann\n\n", args );

    i = 0;
    for( pe = (APTR) partitionlist.lh_Head ; pe->Node.ln_Succ ; pe = (APTR) pe->Node.ln_Succ ) {
        if( pe->PartitionType ) {
            arg = args;
            *arg++ = i++;
            *arg++ = pe->ENV.de_DosType;
            *arg++ = pe->ENV.de_LowCyl;
            *arg++ = pe->ENV.de_HighCyl;
            *arg++ = ((pe->PartitionSize < 1024) ? pe->PartitionSize : ((pe->PartitionSize & 1023) > 800) ? (pe->PartitionSize>>10)+1 : (pe->PartitionSize>>10));
            *arg++ = (ULONG) ((pe->PartitionSize < 1024) ? "MB"              : "GB");
            *arg++ = (ULONG) &pe->FileSysName[0];
            *arg++ = (ULONG) &pe->FileSysName[0];
			VPrintf("%2ld, DosType: %08lx, LowCyl: %10ld HighCyl: %10ld, Size: %4ld %s, FileSystem: %s \n", args );
        }
    }
    VPrintf("\n\n", args);
}
/* \\\ */

/* /// GiggleDisk()
**
*/

/*************************************************************************/

ULONG GiggleDisk( void )
{
ULONG result;
	debug( "\n%76m#\n\n" APPLICATIONNAME ": GiggleDisk V%ld.%ld\n\n", VERSION, REVISION );

    if( !partitionlist.lh_Head ) {
		List_Init( &partitionlist );
    }

	if( !( result = GD_OpenDevice() ) ) {

		if( !(result = Create_PartitionList() ) ) {

/* List if required */

            if( readargs_array[ARG_LIST] || !readargs_array[ARG_TO] ) {
                MD_ShowPartitions();
            }
/* Mount if needed */

            GD_Mount();

/* write dos related files */
            if( (STRPTR) readargs_array[ARG_TO] ) {

                if( Dos_CheckExistsDrawer( (STRPTR) readargs_array[ARG_TO]) ) {
                    result = Create_DosDriver((STRPTR)  readargs_array[ARG_TO], NULL );
                } else {
                    result = Create_MountFile( (STRPTR) readargs_array[ARG_TO], NULL );
                }
            }
        }
    }

    Device_Close( &device );
	List_Free( &partitionlist );

	return( result );
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
