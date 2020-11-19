/*
    Copyright © 2009-2020, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Eject/Load CLI command. 
    Lang: English
*/

/******************************************************************************


    NAME
        Eject <device>

    SYNOPSIS
        DEVICE/A

    LOCATION
       C:

    FUNCTION
        Ejects media from a device. This feature is not supported by all
        device types.

    INPUTS
        DEVICE  --  Name of device to eject media from.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

        Load

    INTERNALS

******************************************************************************/

/******************************************************************************


    NAME
        Load <device>

    SYNOPSIS
        DEVICE/A

    LOCATION
       C:

    FUNCTION
        Loads media into a device. This feature is not supported by all
        device types.

    INPUTS
        DEVICE  --  Name of device to load media into.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

int __nocommandline;

#include <exec/exec.h>
#include <dos/dos.h>
#include <dos/filehandler.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>

#define NAME_BUFFER_SIZE        32
#ifndef ERROR_UNKNOWN
#define ERROR_UNKNOWN           100
#define AROS_BSTR_ADDR(s)       (((STRPTR)BADDR(s))+1)
#endif

#define START_STOP              0x1b
#define ALLOW_MEDIUM_REMOVAL    0x1e

const TEXT version_string[] = "$VER: Eject 41.2 (19.11.2020)";
const TEXT template[] = "DEVICE/A";
const TEXT load_name[] = "Load";


struct Args
{
    TEXT *device;
};


LONG SCSIEject(struct IOStdReq *request, struct MsgPort *port, LONG *result, BOOL load)
{
    UBYTE allowRmBlk[6] = { ALLOW_MEDIUM_REMOVAL, 0, 0, 0, 0, 0 };
    UBYTE startStop1Blk[6] = { START_STOP, 0, 0, 0, 1, 0 };
    UBYTE startStop2Blk[6] = { START_STOP, 0, 0, 0, 2, 0 };

    UBYTE buffer[2];           /* a data buffer used for mode sense data */
    UBYTE Sense[32];        /* buffer for request sense data */
    struct SCSICmd Cmd;      /* where the actual SCSI command goes */
    LONG error = 0;

    Cmd.scsi_Data = (UWORD *)buffer;     /* where we put mode sense data */
    Cmd.scsi_Length = 254;               /* how much we will accept      */
    Cmd.scsi_CmdLength = 6;              /* length of the command        */
    Cmd.scsi_Flags = SCSIF_AUTOSENSE;    /* do automatic REQUEST_SENSE   */
    Cmd.scsi_SenseData = (UBYTE *)Sense; /* where sense data will go     */
    Cmd.scsi_SenseLength = 18;           /* how much we will accept      */
    Cmd.scsi_SenseActual = 0;            /* how much has been received   */

    request->io_Length  = sizeof(struct SCSICmd);
    request->io_Data    = (APTR)&Cmd;
    request->io_Command = HD_SCSICMD;

    Cmd.scsi_Command=(UBYTE *)allowRmBlk; /* issuing ALLOW_MEDIUM_REMOVAL command */
    error = DoIO((struct IORequest *)request);

    Cmd.scsi_Command=(UBYTE *)startStop1Blk; /* issuing START_STOP command */
    DoIO((struct IORequest *)request);

    Cmd.scsi_Command=(UBYTE *)startStop2Blk; /* issuing START_STOP command */
    DoIO((struct IORequest *)request);
    return error;
}

LONG TrackdiskEject(struct IOStdReq *request, struct MsgPort *port, LONG *result, BOOL load)
{
    ULONG signals, received;
    LONG error = 0;

    /* Send command to eject or load media */

    request->io_Command = TD_EJECT;
    request->io_Length = load ? 0 : 1;
    SendIO((struct IORequest *)request);
    signals = (1 << port->mp_SigBit) | SIGBREAKF_CTRL_C;
    received = 0;

    /* Wait for command completion or user break */

    while ((received & SIGBREAKF_CTRL_C) == 0 && GetMsg(port) == NULL)
        received = Wait(signals);
    if ((received & SIGBREAKF_CTRL_C) != 0)
    {
        AbortIO((struct IORequest *)request);
        WaitIO((struct IORequest *)request);
        GetMsg(port);
        error = ERROR_BREAK;
        *result = RETURN_WARN;
    }
    else
    {
        if (request->io_Error != 0)
            error = ERROR_UNKNOWN;
    }
    return error;
}

int main(void)
{
    struct RDArgs *read_args = NULL;
    LONG error = 0, result = RETURN_OK;
    struct Args args = {NULL};
    struct MsgPort *port = NULL;
    struct IOStdReq *request = NULL;
    struct DosList *dos_list;
    TEXT node_name[NAME_BUFFER_SIZE];
    UWORD i;
    struct FileSysStartupMsg *fssm;
    struct DeviceNode *dev_node = NULL;
    BOOL load = FALSE;

    port = CreateMsgPort();
    request =
        (struct IOStdReq *)CreateIORequest(port, sizeof(struct IOStdReq));
    if (port == NULL || request == NULL)
        error = IoErr();

    if (error == 0)
    {
        /* Parse arguments */

        read_args = ReadArgs(template, (APTR)&args, NULL);
        if (read_args == NULL)
        {
            error = IoErr();
            result = RETURN_ERROR;
        }

        /* Find out if we should eject or load media based on name of command */

        load = Strnicmp(FilePart(AROS_BSTR_ADDR(Cli()->cli_CommandName)),
            load_name, sizeof(load_name) - 1) == 0;
    }

    if (error == 0)
    {
        /* Make copy of DOS device name without a colon */

        for (i = 0; args.device[i] != '\0' && args.device[i] != ':'
            && i < NAME_BUFFER_SIZE; i++)
            node_name[i] = args.device[i];
        if (args.device[i] == ':' && i < NAME_BUFFER_SIZE)
            node_name[i] = '\0';
        else
            error = ERROR_INVALID_COMPONENT_NAME;
    }

    /* Determine block device underlying DOS device */

    if (error == 0)
    {
        dos_list = LockDosList(LDF_READ);
        if (dos_list != NULL)
        {
            dev_node = (struct DeviceNode *)FindDosEntry(dos_list, node_name,
                LDF_DEVICES);
            if (dev_node == NULL)
                error = IoErr();
            UnLockDosList(LDF_READ);
        }
        else
            error = IoErr();
    }

    if (error == 0)
    {
        if (IsFileSystem(args.device))
            fssm = (struct FileSysStartupMsg *)BADDR(dev_node->dn_Startup);
        else
            error = ERROR_OBJECT_WRONG_TYPE;
    }

    /* Open block device */

    if (error == 0)
    {
        if (OpenDevice(AROS_BSTR_ADDR(fssm->fssm_Device), fssm->fssm_Unit,
            (struct IORequest *)request, 0) != 0)
            error = ERROR_UNKNOWN;
    }

    if (error == 0)
    {
        error = SCSIEject(request, port, &result, load);
        error = TrackdiskEject(request, port, &result, load);
    }

    /* Deallocate resources */

    if (request != NULL)
        CloseDevice((struct IORequest *)request);
    DeleteIORequest(request);
    DeleteMsgPort(port);

    FreeArgs(read_args);

    /* Print any error message and return */

    SetIoErr(error);
    PrintFault(error, NULL);

    if (error != 0 && result == RETURN_OK)
        result = RETURN_FAIL;

    return result;
}

