/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Copy file to parallel.device
    Lang: English
*/
/*****************************************************************************

    NAME

        CopyToPAR

    SYNOPSIS

        FILE/A,USB/S,QUIET/S

    LOCATION

        C:

    FUNCTION

        Copies (or sends) a file to parallel.device or usbparallel.device.
        
    INPUTS

        FILE   --  Either a file, a directory or a pattern to match.

        USB    --  Use usbparallel.device.

        QUIET  --  Suppresses any output to the shell.

    RESULT

        Standard DOS return codes.

    NOTES

    BUGS

    INTERNALS

******************************************************************************/
#include <exec/io.h>
#include <dos/dos.h>
#include <devices/parallel.h>
#include <proto/exec.h>
#include <proto/dos.h>

/****************************************************************************************/

#define ARG_TEMPLATE        "FILE/A,USB/S,QUIET/S"
#define ARG_FILE            0
#define ARG_USB             1
#define ARG_QUIET           2
#define NUM_ARGS            3

#define BUFSIZE             4096

/****************************************************************************************/

struct MsgPort  *ParMP;
struct IOStdReq *ParIO;
BOOL             ParOpen;
BPTR             fh;
struct RDArgs   *myargs;
IPTR             args[NUM_ARGS];
UBYTE            s[256];
UBYTE            buf[BUFSIZE];
STRPTR           devicename = "parallel.device";

/****************************************************************************************/

static void cleanup(char *msg)
{
    if (msg && !args[ARG_QUIET]) 
    {
        Printf("CopyToPAR: %s\n", msg);
    }
    
    if (fh) Close(fh);
    if (myargs) FreeArgs(myargs);
    
    if (ParOpen) CloseDevice((struct IORequest *)ParIO);
    if (ParIO) DeleteIORequest((struct IORequest *)ParIO);
    if (ParMP) DeleteMsgPort(ParMP);
}

/****************************************************************************************/

static ULONG getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
        Fault(IoErr(), 0, s, 255);
        cleanup(s);
        return RETURN_FAIL;
    }
    return 0;
}

/****************************************************************************************/

static ULONG openpar(void)
{
    ParMP = CreateMsgPort();
    if (!ParMP)
    {
        cleanup("Failed to create msgport");
        return RETURN_ERROR;
    }
    
    ParIO = (struct IOStdReq *)CreateIORequest(ParMP, sizeof(struct IOExtPar));
    if (!ParIO)
    {
        cleanup("Failed to create IO request");
        return RETURN_ERROR;
    }

    if (args[ARG_USB])
    {
        devicename = "usbparallel.device";
    }

    if (OpenDevice(devicename, 0, (struct IORequest *)ParIO, 0))
    {
        cleanup("Failed to open (usb)parallel.device");
        return RETURN_ERROR;
    }

    ParOpen = TRUE;
    return 0;
}

/****************************************************************************************/

static ULONG openfile(void)
{
    fh = Open((STRPTR)args[ARG_FILE], MODE_OLDFILE);
    if (!fh)
    {
        Fault(IoErr(), 0, s, 255);
        cleanup(s);
        return RETURN_FAIL;
    }
    return 0;
}

/****************************************************************************************/

static BOOL WritePAR(APTR buf, ULONG size)
{
    ParIO->io_Command = CMD_WRITE;
    ParIO->io_Data    = buf;
    ParIO->io_Length  = size;
    
    return (DoIO((struct IORequest *)ParIO) == 0) ? TRUE : FALSE;
}

/****************************************************************************************/

static ULONG docopy(void)
{
    LONG size;
    
    do
    {
        size = Read(fh, buf, BUFSIZE);
        if (size == -1)
        {
            Fault(IoErr(), 0, s, 255);
            cleanup(s);
            return RETURN_FAIL;
        }

        if (!WritePAR(buf, size))
        {
            cleanup("Error writing to (usb)parallel.device");
            return RETURN_FAIL;
        }

    } while (size == BUFSIZE);

    return 0;
}

/****************************************************************************************/

int main(void)
{
    int rc;

    rc = getarguments();
    if (rc)
        return rc;

    rc = openpar();
    if (rc)
        return rc;

    rc = openfile();
    if (rc)
        return rc;

    rc = docopy();
    if (rc)
        return rc;

    cleanup(NULL);    
    return 0;
}
