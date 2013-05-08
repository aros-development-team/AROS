/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        Print

    SYNOPSIS

        FILE/M
        PRTUNIT/N

    LOCATION

        C:

    FUNCTION

        Print a file, using datatypes.library

    INPUTS

        FILE f      --  Filename to print
        PRTUNIT n   --  Printer unit

    RESULT

        Files will be printed to the specified printer unit

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/datatypes.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <aros/shcommands.h>

static inline BOOL isPrintable(struct Library *DataTypesBase, Object *obj)
{
    ULONG *dtm;

    for (dtm = GetDTMethods(obj); (SIPTR)dtm != -1; dtm++) {
        if (*dtm == DTM_PRINT)
            return TRUE;
    }

    return FALSE;
}

ULONG doPrinting(struct ExecBase *SysBase, struct Library *DataTypesBase, Object *dto, int unit)
{
    struct dtPrint msg;
    struct MsgPort *mp;
    struct IORequest *pio;
    ULONG retval = PDERR_CANCEL;
    struct Process *pr = (struct Process *)FindTask(NULL);
   
    if ((mp = CreateMsgPort())) {
        if ((pio = CreateIORequest(mp, sizeof(union printerIO)))) {
            if (0 == OpenDevice("printer.device", unit, pio, 0)) {
                msg.MethodID          = DTM_PRINT;
                msg.dtp_GInfo         = NULL;
                msg.dtp_PIO           = (union printerIO *)pio;
                msg.dtp_AttrList      = NULL;

                retval = DoDTMethodA(dto, pr->pr_WindowPtr, NULL, (Msg)&msg);

                CloseDevice(pio);
            } else {
                retval = -13;
            }
            DeleteIORequest(pio);
        } else {
            retval = -14;
        }
        DeleteMsgPort(mp);
    } else {
        retval = -15;
    }

    return retval;
}


AROS_SH2H(Print , 44.0,               "Print a file using DataTypes\n",
AROS_SHAH(LONG *  , ,PRTUNIT,/K/N,   0 ,  "Printer unit\n"),
AROS_SHAH(STRPTR *, ,FILES  ,/M,  NULL ,  "File(s) to print\n"))
{
    AROS_SHCOMMAND_INIT

    struct Library *DataTypesBase;
    STRPTR *files = SHArg(FILES);
    LONG unit = SHArg(PRTUNIT) ? *SHArg(PRTUNIT) : 0;
    LONG err;
  
    SetIoErr(RETURN_FAIL);

    if (files != NULL) {
        if ((DataTypesBase = OpenLibrary("datatypes.library", 0))) {
            Object *o;
            SetIoErr(RETURN_OK);
            for (; *files; files++) {
                STRPTR file = *files;
                if ((o = NewDTObject(file, PDTA_Remap, FALSE, TAG_END))) {
                    if (isPrintable(DataTypesBase, o)) {
                        err = doPrinting(SysBase, DataTypesBase, o, unit);
                        if (err) {
                            Printf("Can't print \"%s\": Printer Error %d\n", file, err);
                            SetIoErr(RETURN_FAIL);
                        }
                    } else {
                        Printf("\"%s\" is not a DataType printable file\n", file);
                        SetIoErr(RETURN_FAIL);
                    }
                    DisposeDTObject(o);
                } else {
                    Printf("Can't open %s as a DataType object\n", file);
                }
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
