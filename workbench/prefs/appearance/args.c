/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <dos/rdargs.h>
#include <workbench/startup.h>

#include <proto/dos.h>
#include <proto/icon.h>

#include "args.h"
#include "misc.h"

/*** Global Variables *******************************************************/
STATIC CONST_STRPTR   TEMPLATE = "FROM,USE/S,SAVE/S";
STATIC IPTR           args[COUNT];
STATIC struct RDArgs *rdargs;
STATIC BPTR           olddir = (BPTR)-1;

/*** Functions **************************************************************/
BOOL ReadArguments(int argc, char **argv)
{
    if (argc)
    {
        rdargs = ReadArgs(TEMPLATE, args, NULL);
        return rdargs != NULL;
    }
    else
    {
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;
        struct DiskObject *dobj;
        STRPTR *toolarray;
        STRPTR tooltype;

        if (wbmsg->sm_NumArgs > 1)
        {
            wbarg++;
            if (wbarg->wa_Lock && *wbarg->wa_Name)
            {
                olddir = CurrentDir(wbarg->wa_Lock);
                dobj = GetDiskObject(wbarg->wa_Name);
                if (dobj)
                {
                    args[FROM] = (IPTR)wbarg->wa_Name;
                    toolarray = dobj->do_ToolTypes;
                    tooltype = FindToolType(toolarray, "ACTION");
                    if (tooltype)
                    {
                        if (MatchToolValue(tooltype, "USE"))
                        {
                            args[USE] = TRUE;
                        }
                        else if (MatchToolValue(tooltype, "SAVE"))
                        {
                            args[SAVE] = TRUE;
                        }
                    }
                    FreeDiskObject(dobj);
                    return TRUE;
                }
                else
                {
                    ShowMessage("Couldn't read DiskObject\n");
                }
            }
        }
        else if (wbmsg->sm_NumArgs == 1)
        {
            return TRUE;
        }
    }
    return FALSE;
}

VOID FreeArguments(VOID)
{
    FreeArgs(rdargs);

    if (olddir != (BPTR)-1)
        CurrentDir(olddir);
}

IPTR GetArgument(enum Argument id)
{
    if ((id >= 0) && (id < COUNT))
	return args[id];

    return 0;
}
