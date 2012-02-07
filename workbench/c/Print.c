/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Print

    SYNOPSIS

        QUIET/S,UNIT/K/N,FILE/S

    LOCATION

        C:

    FUNCTION

        Print a file, using datatypes.library

    INPUTS

        QUIET    --  Don't indicate when the print is complete

        UNIT=n   --  printer.device unit to print to

        FILE f   --  Filename to print

    RESULT

        Unless QUIET, print out when a print job has been completed:

        [<filename> printed on printer.device <n>]


    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <exec/memory.h>
#include <datatypes/datatypesclass.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/datatypes.h>

#define DEBUG 0
#include <aros/debug.h>

#include <aros/shcommands.h>

AROS_SH3H(Print, 44.0,              "Print a file using DataTypes\n",
AROS_SHAH(BOOL  , ,QUIET  ,/S,FALSE,  "Don't say when the print is complete"),
AROS_SHAH(ULONG *, ,UNIT  ,/N/K,NULL, "Select the printer.device unit (default 0)"),
AROS_SHAH(STRPTR, ,FILE   ,  ,NULL ,  "File to print\n") )
{
    AROS_SHCOMMAND_INIT

    struct Library *DataTypesBase;
    BOOL quiet = SHArg(QUIET);
    ULONG unit = (SHArg(UNIT) ? *SHArg(UNIT) : 0);
    STRPTR file = SHArg(FILE);
  
    SetIoErr(RETURN_FAIL);

    if (file != NULL) {
        if ((DataTypesBase = OpenLibrary("datatypes.library", 0))) {
            Object *o;
            if ((o = NewDTObject(file, TAG_END))) {
                struct dtPrint msg;
                struct MsgPort *mp;
                struct IORequest *io;
                if ((mp = CreateMsgPort())) {
                    if ((io = CreateIORequest(mp, sizeof(union printerIO)))) {
                        if (0 == OpenDevice("printer.device", unit, io, 0)) {
                            msg.MethodID          = DTM_PRINT;
                            msg.dtp_GInfo         = NULL;
                            msg.dtp_PIO           = (union printerIO *)io;
                            msg.dtp_AttrList      = NULL;
                            if (0 == DoDTMethodA(o, NULL, NULL, (Msg)&msg)) {
                                if (!quiet)
                                    Printf("[%s printed to printer.device %ld]\n", file, unit);
                                SetIoErr(0);
                            } else {
                                if (!quiet)
                                    Printf("[%s failed to print to printer.device %ld]\n", file, unit);
                                SetIoErr(RETURN_FAIL);
                            }
                            CloseDevice(io);
                        } else {
                            Printf("Can't open printer.device %ld\n", unit);
                        }
                        DeleteIORequest(io);
                    } else {
                        Printf("Can't create IO request\n");
                    }
                    DeleteMsgPort(mp);
                } else {
                    Printf("Can't create message port\n");
                }
                DisposeDTObject(o);
            } else {
                Printf("Can't open %s as a DataType object\n", file);
            }
            CloseLibrary(DataTypesBase);
        } else {
            Printf("Can't open datatypes.library\n");
        }
    } else {
        /* No file supplied - quiet success */
        SetIoErr(0);
    }

    return IoErr();

    AROS_SHCOMMAND_EXIT
}
 
