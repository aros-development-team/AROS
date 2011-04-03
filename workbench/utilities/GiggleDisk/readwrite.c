
/*
** readwrite.c
**
** (c) 1998-2011 Guido Mersmann
*/

/*************************************************************************/

#define SOURCENAME "readwrite.c"
#define NODEBUG  /* turns off debug of this file */

#include <internal/debug.h>
#include <internal/memory.h>

#include <proto/exec.h>

#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <exec/io.h>

#include "functions.h"
#include "header.h"
#include "locale_strings.h"
#include "requester.h"

/*************************************************************************/

/* /// Device_ReadBlock()
**
*/

/*************************************************************************/

ULONG Device_ReadBlock( UBYTE *buffer, ULONG block, ULONG secsize)
{

    if( !device64 ) {
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Command = CMD_READ;
    } else {
        ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Command = NSCMD_TD_READ64;
    }
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Data = buffer;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Length = secsize;


    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Actual  = block>>(32-9); /* high 32 */
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Offset = block <<9; /* low 32 */
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Flags |= IOF_QUICK;
    ((struct IOExtTD *) device.IORequest)->iotd_Req.io_Length = secsize;

    SendIO( device.IORequest );

    Wait( (1<< (device.IORequest->io_Message.mn_ReplyPort->mp_SigBit)) | SIGBREAKF_CTRL_C );

    if( !CheckIO( device.IORequest ) ) {
        AbortIO( device.IORequest );
        WaitIO( device.IORequest );
    }

    if( !device.IORequest->io_Error ) {
        return( 0L);
    } else {
    requester_args[0] = block;
    requester_args[1] = secsize;
    return( MSG_ERROR_UnableToReadBlock );
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      


/* \\\ */



