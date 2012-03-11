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
#include <proto/icon.h>

#include <workbench/startup.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

//#define DEBUG 1
#include <aros/debug.h>

#define USAGE "Usage: PrintFiles [-f] [-u N] file [file] [file...] (-f=formfeed -u=unit number)\n"

const char *vers = "$VER: PrintFiles 1.1 (12.03.2012)";

static struct MsgPort *mp;
static union printerIO *io;

char __stdiowin[]="CON:/30/400/100/PrintFiles/AUTO/CLOSE/WAIT";


static BOOL initdevice(ULONG unit)
{
    D(bug("[PrintFiles] init unit %d\n", unit));

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
            DisposeDTObject(o);
        }
    }
}


static void read_icon(struct WBArg *wbarg, BOOL *formfeed, ULONG *unit)
{
    struct DiskObject *dobj;
    STRPTR *toolarray;
    STRPTR result;

    *formfeed = FALSE;
    *unit = 0;

    dobj = GetDiskObject(wbarg->wa_Name);
    if (dobj)
    {
        toolarray = dobj->do_ToolTypes;

        if (FindToolType(toolarray, "FORMFEED"))
        {
            *formfeed = TRUE;
        }
        result = FindToolType(toolarray, "UNIT");
        if (result)
        {
            StrToLong(result, unit);
        }
        FreeDiskObject(dobj);
    }
}


int main(int argc, char **argv)
{
    ULONG unit = 0;
    BOOL formfeed = FALSE;
    ULONG i;

    if (argc == 0)
    {
        // started from Workbench
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;
        BPTR olddir = (BPTR)-1;

        D(bug("[PrintFiles] numargs %d wa_lock %lx wa_name %s\n", wbmsg->sm_NumArgs, wbarg[0].wa_Lock, wbarg[0].wa_Name));
        if (wbmsg->sm_NumArgs > 1 && wbarg[0].wa_Lock && *wbarg[0].wa_Name)
        {
            // handle program's icon
            olddir = CurrentDir(wbarg->wa_Lock);
            read_icon(wbarg, &formfeed, &unit);
            if (olddir != (BPTR)-1)
                CurrentDir(olddir);
            if (initdevice(unit))
            {
                // handle project icons
                for (i = 1; i < wbmsg->sm_NumArgs; i++)
                {
                    D(bug("[PrintFiles] i %d wa_lock %lx wa_name %s\n", i, wbarg[i].wa_Lock, wbarg[i].wa_Name));
                    olddir = (BPTR)-1;
                    if ((wbarg[i].wa_Lock) && (*wbarg[i].wa_Name) )
                    {
                        olddir = CurrentDir(wbarg[i].wa_Lock);

                        printfile(wbarg[i].wa_Name, formfeed);
                        
                        if (olddir != (BPTR)-1)
                            CurrentDir(olddir);
                    }
                }
            }
        }
    }
    else
    {
        // started from CLI
        
        if (argc == 1 || argv[1][0] == '?')
        {
            PutStr(USAGE);
            return RETURN_ERROR;
        }

        // read options
        i = 1;
        while (i < argc && argv[i][0] == '-')
        {
            if (argv[i][1] == 'f')
            {
                formfeed = TRUE;
            }
            else if (argv[i][1] == 'u' && (i + 1 < argc))
            {
                i++;
                StrToLong(argv[i], &unit);
            }
            else
            {
                PutStr(USAGE);
                return RETURN_ERROR;
            }
            i++;
        }

        // print files
        if (initdevice(unit))
        {
            while (i < argc)
            {
                printfile(argv[i], formfeed);
                i++;
            }
        }
    }

    cleanupdevice();

    return 0;
}
