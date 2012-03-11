/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Dump screen to printer
    Lang: English
*/

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/ports.h>
#include <devices/printer.h>
#include <devices/prtbase.h>
#include <dos/dos.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/displayinfo.h>
#include <workbench/startup.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/alib.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <stdlib.h>

const char *vers = "$VER: GraphicDump 1.0 (11.03.2012)";

#define ARG_TEMPLATE "SIZE/K,TINY/S,SMALL/S,MEDIUM/S,LARGE/S,DELAY/N/K,UNIT/N/K,DOTS"

enum
{
    ARG_SIZE,
    ARG_TINY,
    ARG_SMALL,
    ARG_MEDIUM,
    ARG_LARGE,
    ARG_DELAY,
    ARG_UNIT,
    ARG_DOTS,
    ARG_COUNT
};

enum
{
    SIZE_TINY,  // 1/4
    SIZE_SMALL, // 1/2
    SIZE_MEDIUM,// 3/4
    SIZE_LARGE  // 1/1
};


#if 0
/* Possible printer.device and I/O errors */
static UBYTE *ErrorText[] =
{
    "PDERR_NOERR",
    "PDERR_CANCEL",
    "PDERR_NOTGRAPHICS",
    "INVERTHAM",                /* OBSOLETE */
    "BADDIMENSION",
    "DIMENSIONOVFLOW",  /* OBSOLETE */
    "INTERNALMEMORY",
    "BUFFERMEMORY",
    /* IO_ERRs */
    "IOERR_OPENFAIL",
    "IOERR_ABORTED",
    "IOERR_NOCMD",
    "IOERR_BADLENGTH"
};
#endif

void dump(ULONG unit, ULONG size)
{
    struct MsgPort  *PrinterMP;
    union printerIO *PIO;
    struct Screen *pubscreen;
    struct ViewPort *vp;
    LONG modeID;
    ULONG signal;

    if ((PrinterMP = CreateMsgPort()) != NULL)
    {
        if ((PIO = (union printerIO *)CreateExtIO(PrinterMP, sizeof(union printerIO))) != NULL)
        {
            if (!(OpenDevice("printer.device", 0, (struct IORequest *)PIO, unit)))
            {
                // TODO: use frontmost screen
                if ((pubscreen = LockPubScreen(NULL)) != NULL)
                {
                    vp = &(pubscreen->ViewPort);
                    if ((modeID = GetVPModeID(vp)) != INVALID_ID)
                    {
                        PIO->iodrp.io_Command = PRD_DUMPRPORT;
                        PIO->iodrp.io_RastPort = &(pubscreen->RastPort);
                        PIO->iodrp.io_ColorMap = vp->ColorMap;
                        PIO->iodrp.io_Modes = modeID;
                        PIO->iodrp.io_SrcX = pubscreen->LeftEdge;
                        PIO->iodrp.io_SrcY = pubscreen->TopEdge;
                        PIO->iodrp.io_SrcWidth = pubscreen->Width;
                        PIO->iodrp.io_SrcHeight = pubscreen->Height;
#if 1
                        // FIXME: doesn't work
                        // I'm getting an empty sheet with the postscript driver
                        PIO->iodrp.io_Special = SPECIAL_ASPECT | SPECIAL_FRACCOLS;
                        PIO->iodrp.io_DestCols = 0xffff / 2;
                        PIO->iodrp.io_DestRows = 0;
#else
                        // FIXME: doesn't work
                        // It prints on the full paper height
                        PIO->iodrp.io_Special = SPECIAL_ASPECT;
                        PIO->iodrp.io_DestCols = 300;
                        PIO->iodrp.io_DestRows = 0;
#endif
                        SendIO((struct IORequest *)PIO);
                        signal = Wait(1 << PrinterMP->mp_SigBit | SIGBREAKF_CTRL_C);
                        if (signal & SIGBREAKF_CTRL_C)
                        {
                            AbortIO((struct IORequest *)PIO);
                            WaitIO((struct IORequest *)PIO);
                        }
                        if (signal & (1 << PrinterMP->mp_SigBit))
                        {
                            while (GetMsg(PrinterMP)) ;
                        }
                        if (PIO->iodrp.io_Error != 0)
                        {
                            Printf("GraphicsDump: Error %ld\n", PIO->iodrp.io_Error);
                        }
                    }
                    else
                    {
                        PutStr("GraphicsDump: Invalid ModeID\n");
                    }
                    UnlockPubScreen(NULL, pubscreen);
                }
                else
                {
                    PutStr("GraphicsDump: Can't lock Public Screen\n");
                }
                CloseDevice((struct IORequest *)PIO);
            }
            else
            {
                PutStr("GraphicsDump: Can't open printer.device\n");
            }
            DeleteExtIO((struct IORequest *)PIO);
        }
        else
        {
            PutStr("GraphicsDump: Can't create Extented I/O Request\n");
        }
        DeleteMsgPort(PrinterMP);
    }
    else
    {
        PutStr("GraphicsDump: Can't create Message port\n");
    }
}


static void read_icon(struct WBArg *wbarg, ULONG *unit, ULONG *delay, ULONG *size)
{
    struct DiskObject *dobj;
    STRPTR *toolarray;
    STRPTR result;

    dobj = GetDiskObject(wbarg->wa_Name);
    if (dobj)
    {
        toolarray = dobj->do_ToolTypes;
        result = FindToolType(toolarray, "SIZE");
        if (result)
        {
            if (Stricmp(result, "TINY") == 0)
            {
                *size = SIZE_TINY;
            }
            else if (Stricmp(result, "SMALL") == 0)
            {
                *size = SIZE_SMALL;
            }
            else if (Stricmp(result, "MEDIUM") == 0)
            {
                *size = SIZE_MEDIUM;
            }
            else if (Stricmp(result, "LARGE") == 0)
            {
                *size = SIZE_LARGE;
            }
            else
            {
                // TODO: check for x:y
            }
        }
        if (FindToolType(toolarray, "TINY"))
        {
            *size = SIZE_TINY;
        }
        if (FindToolType(toolarray, "SMALL"))
        {
            *size = SIZE_SMALL;
        }
        if (FindToolType(toolarray, "MEDIUM"))
        {
            *size = SIZE_MEDIUM;
        }
        if (FindToolType(toolarray, "LARGE"))
        {
            *size = SIZE_LARGE;
        }
        result = FindToolType(toolarray, "DELAY");
        if (result)
        {
            *delay = atoi(result);
        }
        result = FindToolType(toolarray, "UNIT");
        if (result)
        {
            *unit = atoi(result);
        }
        result = FindToolType(toolarray, "DOTS");
        if (result)
        {
            // TODO: implement
        }
        FreeDiskObject(dobj);
    }
}


int main(int argc, char **argv)
{
    ULONG unit = 0;
    ULONG delay = 10;
    ULONG size = SIZE_LARGE;

    if (argc == 0)
    {
        // started from Wanderer
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;
        BPTR olddir = (BPTR)-1;

        if ((wbmsg->sm_NumArgs > 0) && (wbarg[0].wa_Lock) && (*wbarg[0].wa_Name))
        {
            olddir = CurrentDir(wbarg[0].wa_Lock);
            read_icon(wbarg, &unit, &delay, &size);
            if (olddir != (BPTR)-1)
            {
                CurrentDir(olddir);
            }
        }
    }
    else
    {
        // started from CLI
        IPTR args[ARG_COUNT] = {0};

        struct RDArgs *rda = ReadArgs(ARG_TEMPLATE, args, NULL);
        if (!rda)
        {
            PrintFault(IoErr(), argv[0]);
            return RETURN_ERROR;
        }

        if (args[ARG_SIZE])
        {
            if (Stricmp((STRPTR)args[ARG_SIZE], "TINY") == 0)
            {
                size = SIZE_TINY;
            }
            else if (Stricmp((STRPTR)args[ARG_SIZE], "SMALL") == 0)
            {
                size = SIZE_SMALL;
            }
            else if (Stricmp((STRPTR)args[ARG_SIZE], "MEDIUM") == 0)
            {
                size = SIZE_MEDIUM;
            }
            else if (Stricmp((STRPTR)args[ARG_SIZE], "LARGE") == 0)
            {
                size = SIZE_LARGE;
            }
            else
            {
                // TODO: check for x:y
            }
        }
        if (args[ARG_TINY])
        {
            size = SIZE_TINY;
        }
        if (args[ARG_SMALL])
        {
            size = SIZE_SMALL;
        }
        if (args[ARG_MEDIUM])
        {
            size = SIZE_MEDIUM;
        }
        if (args[ARG_LARGE])
        {
            size = SIZE_LARGE;
        }
        if (args[ARG_DELAY])
        {
            delay = *(LONG *)args[ARG_DELAY];
        }
        if (args[ARG_UNIT])
        {
            unit = *(LONG *)args[ARG_UNIT];
        }
        if (args[ARG_DOTS])
        {
            // TODO: implement
        }
        FreeArgs(rda);
    }

    Delay(delay * 50);

    dump(unit, size);

    return 0;
}
