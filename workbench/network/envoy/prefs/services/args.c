/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Envoy Services preferences - arguments, from the shell
          (FROM,USE/S,SAVE/S,PUBSCREEN/K) or from Workbench (a project icon
          with ACTION=USE or ACTION=SAVE), as the other prefs programs do.
*/

#include <dos/rdargs.h>
#include <workbench/startup.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include "args.h"

static CONST_STRPTR TEMPLATE = "FROM,USE/S,SAVE/S,PUBSCREEN/K";
static IPTR args[COUNT];
static struct RDArgs *rdargs;
static BPTR olddir = (BPTR)-1;

BOOL ReadArguments(int argc, char **argv)
{
    if (argc)
    {
        if (!(rdargs = ReadArgs(TEMPLATE, args, NULL)))
        {
            PrintFault(IoErr(), "Services");
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        struct WBStartup *wbmsg = (struct WBStartup *)argv;
        struct WBArg *wbarg = wbmsg->sm_ArgList;
        struct DiskObject *dobj;
        STRPTR tooltype;

        if (wbmsg->sm_NumArgs > 1)
        {
            wbarg++;
            if (wbarg->wa_Lock && *wbarg->wa_Name)
            {
                olddir = CurrentDir(wbarg->wa_Lock);
                if ((dobj = GetDiskObject(wbarg->wa_Name)))
                {
                    args[FROM] = (IPTR)wbarg->wa_Name;
                    if ((tooltype = FindToolType(dobj->do_ToolTypes, "ACTION")))
                    {
                        if (MatchToolValue(tooltype, "USE"))
                            args[USE] = TRUE;
                        else if (MatchToolValue(tooltype, "SAVE"))
                            args[SAVE] = TRUE;
                    }
                    FreeDiskObject(dobj);
                    return TRUE;
                }
            }
            return FALSE;
        }
        return TRUE;
    }
}

VOID FreeArguments(VOID)
{
    if (rdargs)
        FreeArgs(rdargs);
    if (olddir != (BPTR)-1)
        CurrentDir(olddir);
}

IPTR GetArgument(enum Argument id)
{
    return (id >= 0 && id < COUNT) ? args[id] : 0;
}
