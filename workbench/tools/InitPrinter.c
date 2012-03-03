/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <devices/printer.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/exec.h>

//#define DEBUG 1
#include <aros/debug.h>

const char version[] = "$VER: InitPrinter 1.0 (03.03.2012) © AROS Dev Team";

char __stdiowin[]="CON:/30/400/100/InitPrinter/AUTO/CLOSE/WAIT";

enum
{
    ARG_UNIT,
    ARG_COUNT
};

static BOOL init_printer(ULONG unit)
{
    struct MsgPort *PrintMP;
    struct IOPrtCmdReq *PrintIO;
    BOOL success = FALSE;

    if ((PrintMP = CreateMsgPort()))
    {
        if ((PrintIO = CreateIORequest(PrintMP, sizeof(struct IOPrtCmdReq))))
        {
            if (OpenDevice("printer.device", unit, (struct IORequest *)PrintIO, 0))
            {
                PutStr("Error: printer.device did not open\n");
            }
            else
            {
                D(bug("[InitPrinter] unit %d request %p msgport %p\n", unit, PrintIO, PrintMP));
                PrintIO->io_PrtCommand = aRIN;     // Initialize
                PrintIO->io_Parm0 = 0;
                PrintIO->io_Parm1 = 0;
                PrintIO->io_Parm2 = 0;
                PrintIO->io_Parm3 = 0;
                PrintIO->io_Command = PRD_PRTCOMMAND;

                if (DoIO((struct IORequest *)PrintIO))
                {
                    Printf("Printer reset failed. Error: %d\n", (IPTR)PrintIO->io_Error);
                }
                else
                {
                    success = TRUE;
                }
                CloseDevice((struct IORequest *)PrintIO);
            }
            DeleteIORequest(PrintIO);
        }
        else
        {
            PutStr("Error: Could not create I/O request\n");
        }
        DeleteMsgPort(PrintMP);
    }
    else
    {
        PutStr("Error: Could not create message port\n");
    }
    return success;
}


int main(void)
{
    struct RDArgs *rda;
    IPTR args[ARG_COUNT] = {0};
    ULONG unit = 0;
    ULONG retval = RETURN_ERROR;

    if ((rda = ReadArgs("UNIT/N", args, NULL)))
    {
        if (args[ARG_UNIT])
        {
            unit = *(LONG *)args[ARG_UNIT];
        }
        if (init_printer(unit))
        {
            retval = RETURN_OK;
        }
        FreeArgs(rda);
    }
    else
    {
        PutStr("Error: Could not read arguments\n");
    }

    return retval;
}
