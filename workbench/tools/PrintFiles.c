/*
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Print multiple files with optional formfeed
    Lang: English
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/datatypes.h>

#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#define DEBUG 1
#include <aros/debug.h>

const char *vers = "$VER: PrintFiles 1.0 (07.03.2012)";

static struct MsgPort *mp;
static union printerIO *io;


static BOOL initdevice(ULONG unit)
{
    if ((mp = CreateMsgPort()))
    {
        if ((io = CreateIORequest(mp, sizeof(union printerIO))))
        {
            if (0 == OpenDevice("printer.device", unit, (struct IORequest *)io, 0))
            {
                return TRUE;
            }
            else
            {
                Printf("Can't open printer.device %ld\n", unit);
            }
        }
        else
        {
            PutStr("Can't create IO request\n");
        }
    }
    else
    {
        PutStr("Can't create message port\n");
    }
    return FALSE;
}


static void cleanupdevice(void)
{
    if (io)
    {
        CloseDevice((struct IORequest *)io);
        DeleteIORequest(io);
        io = NULL;
    }
    if (mp)
    {
        DeleteMsgPort(mp);
        mp = NULL;
    }
}


static void printfile(STRPTR filename, BOOL formfeed)
{
    if (filename != NULL)
    {
        Object *o;
        struct dtPrint msg;
        if ((o = NewDTObject(filename, PDTA_Remap, FALSE, TAG_END)))
        {
            struct TagItem tags[] = {
                { DTA_Special, SPECIAL_ASPECT | SPECIAL_CENTER },
                { TAG_END }
            };
            msg.MethodID          = DTM_PRINT;
            msg.dtp_GInfo         = NULL;
            msg.dtp_PIO           = (union printerIO *)io;
            msg.dtp_AttrList      = tags;
            D(bug("[PrintFiles] Trying to print %s\n", filename));
            if (0 == DoDTMethodA(o, NULL, NULL, (Msg)&msg))
            {
                if (formfeed)
                {
                    D(bug("[PrintFiles] Sending formfeed\n"));
                    io->ios.io_Length  = 1;
                    io->ios.io_Data = "\x0C";
                    io->ios.io_Command = CMD_WRITE;
                    DoIO((struct IORequest *)io);
                }
            }
            else
            {
                Printf("Failed to print %s\n", filename);
            }
        }
    }
}


int main(int argc, char **argv)
{
    ULONG unit = 0;
    BOOL formfeed = FALSE;
    ULONG i, first;

    if (argc == 0)
    {
        // started from Workbench
        // TODO: implement
    }
    else
    {
        // started from CLI
        if (argc > 1 && argv[1][0] != '?')
        {
            first = 1;
            if (strcmp(argv[1], "-f") == 0)
            {
                first = 2;
                formfeed = TRUE;
            }
            if (initdevice(unit))
            {
                for (i = first; i < argc; i++)
                {
                    printfile(argv[i], formfeed);
                }
            }
            cleanupdevice();
        }
        else
        {
            PutStr("Usage: PrintFiles [-f] file [file] [file...] (-f=formfeed)\n");
        }
    }

    return 0;
}
