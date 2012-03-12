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

//#define DEBUG 1
#include <aros/debug.h>


const char *vers = "$VER: GraphicDump 1.1 (12.03.2012)";

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
    SIZE_LARGE, // 1/1
    SIZE_DOTS,  // X:Y
    SIZE_COUNT
};

// scale factors as fraction x/0xffffffff
static ULONG scale[] =
{
    0x3fffffff, // TINY
    0x7fffffff, // SMALL
    0xbfffffff, // MEDIUM
    0xffffffff, // LARGE
    0           // DOTS
};

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

// copy the bitmap of the frontmost screen
// return value must be freed with FreeBitMap()
static struct BitMap *dup_firstscreen_bitmap(struct Screen **screen)
{
    struct BitMap *newbm = NULL;

    ULONG ilock = LockIBase(0);
    *screen = IntuitionBase->FirstScreen;
    if (*screen)
    {
        newbm = AllocBitMap
        (
            (*screen)->Width, (*screen)->Height, 0,
            BMF_MINPLANES, (*screen)->RastPort.BitMap
        );
        if (newbm)
        {
            BltBitMap
            (
                (*screen)->RastPort.BitMap, 0, 0, newbm,
                0, 0, (*screen)->Width, (*screen)->Height,
                0xC0, 0xff, NULL
            );
        };
    }
    UnlockIBase(ilock);
    D(bug("[GraphicDump/dup_firstscreen_bitmap] screen %p bitmap %p width %d height %d\n",
        *screen, newbm, (*screen)->Width, (*screen)->Height));

    return newbm;
}


static void dump(ULONG unit, ULONG size, ULONG width, ULONG height)
{
    struct MsgPort  *PrinterMP;
    union printerIO *PIO;
    struct Screen *screen;
    struct BitMap *screenbitmap;
    struct ViewPort *viewport;
    LONG modeID;
    ULONG signal;

    // sanity
    if (size > SIZE_COUNT)
        size = SIZE_SMALL;

    D(bug("[GraphicDump/dump] unit %u size %u\n", unit, size));

    if ((PrinterMP = CreateMsgPort()) != NULL)
    {
        if ((PIO = (union printerIO *)CreateExtIO(PrinterMP, sizeof(union printerIO))) != NULL)
        {
            if (!(OpenDevice("printer.device", 0, (struct IORequest *)PIO, unit)))
            {
                if ((screenbitmap = dup_firstscreen_bitmap(&screen)) != NULL)
                {
                    viewport = &(screen->ViewPort);
                    if ((modeID = GetVPModeID(viewport)) != INVALID_ID)
                    {
                        PIO->iodrp.io_Command = PRD_DUMPRPORT;
                        PIO->iodrp.io_RastPort = &(screen->RastPort);
                        PIO->iodrp.io_ColorMap = viewport->ColorMap;
                        PIO->iodrp.io_Modes = modeID;
                        PIO->iodrp.io_SrcX = screen->LeftEdge;
                        PIO->iodrp.io_SrcY = screen->TopEdge;
                        PIO->iodrp.io_SrcWidth = screen->Width;
                        PIO->iodrp.io_SrcHeight = screen->Height;

                        if (size == SIZE_DOTS)
                        {
                            PIO->iodrp.io_DestCols = width;
                            PIO->iodrp.io_DestRows = height;
                        }
                        else
                        {
                            PIO->iodrp.io_Special = SPECIAL_ASPECT | SPECIAL_FRACCOLS;
                            PIO->iodrp.io_DestCols = scale[size];
                            PIO->iodrp.io_DestRows = 0;
                        }

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
                            Printf("GraphicsDump: Error %s\n", ErrorText[PIO->iodrp.io_Error]);
                        }
                    }
                    else
                    {
                        PutStr("GraphicsDump: Invalid ModeID\n");
                    }
                    FreeBitMap(screenbitmap);
                }
                else
                {
                    PutStr("GraphicsDump: Can't copy screen bitmap\n");
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


// split a string with the pattern x:y
// if no colon exists the actual values are kept
static CONST_STRPTR split_xy(CONST_STRPTR string, ULONG *x, ULONG *y)
{
    // search for :
    const char *colon = NULL;
    const char *str = string;
    do
    {
        if ((unsigned char)*str == ':')
        {
            colon = str;
        }
    } while (*(str++));

    // get x and y value
    if (colon)
    {
        StrToLong(string, x);
        StrToLong(colon + 1, y);
    }

    D(bug("[GraphicDump/split_xy] x %u y %u\n", *x, *y));

    return colon;
}


// parse tooltypes
static void read_icon(struct WBArg *wbarg, ULONG *unit, ULONG *delay, ULONG *size, ULONG *width, ULONG *height)
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
                *size = SIZE_DOTS;
                split_xy(result, width, height);
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
            StrToLong(result, delay);
        }
        result = FindToolType(toolarray, "UNIT");
        if (result)
        {
            StrToLong(result, unit);
        }
        result = FindToolType(toolarray, "DOTS");
        if (result)
        {
            *size = SIZE_DOTS;
            split_xy(result, width, height);
        }
        FreeDiskObject(dobj);
    }
}


int main(int argc, char **argv)
{
    ULONG unit = 0;
    ULONG delay = 10;
    ULONG size = SIZE_SMALL;
    ULONG width = 300;
    ULONG height = 300;

    if (argc == 0)
    {
        // started from Wanderer
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;
        BPTR olddir = (BPTR)-1;

        if ((wbmsg->sm_NumArgs > 0) && (wbarg[0].wa_Lock) && (*wbarg[0].wa_Name))
        {
            olddir = CurrentDir(wbarg[0].wa_Lock);
            read_icon(wbarg, &unit, &delay, &size, &width, &height);
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
                size = SIZE_DOTS;
                split_xy((CONST_STRPTR)args[ARG_SIZE], &width, &height);
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
            size = SIZE_DOTS;
            split_xy((CONST_STRPTR)args[ARG_DOTS], &width, &height);
        }
        FreeArgs(rda);
    }

    Delay(delay * 50);

    dump(unit, size, width, height);

    return 0;
}
