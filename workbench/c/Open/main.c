/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Open CLI command
    Lang: English
*/


#include <exec/types.h>
#include <dos/dos.h>

#include <proto/dos.h>
#include <proto/workbench.h>
#include <proto/utility.h>

#define DEBUG 0
#include <aros/debug.h>

#define SH_GLOBAL_DOSBASE TRUE
#include <aros/shcommands.h>

AROS_SH2H
(
    Open, 0.3,                              "Open/launch a workbench object as if its icon was double-clicked",
    AROS_SHAH(STRPTR  , , NAME     , /A, "","\tName of the object to be opened"),
    AROS_SHAH(STRPTR *, , ARGUMENTS, /M, NULL,"List of parameters (Workbench arguments)\n"
                                      "\t\t\t\tto pass to the tool/project to be launched")
)
{
    AROS_SHCOMMAND_INIT

    struct Library *WorkbenchBase = OpenLibrary("workbench.library", 44L);
    struct Library *UtilityBase   = OpenLibrary("utility.library", 41L);
    struct TagItem *argsTagList   = NULL;
    BOOL            success       = FALSE;
    int             nbArgs        = 0, i = 0;
    STRPTR         *wbArg;
    
    if (WorkbenchBase != NULL)
    {
        if (SHArg(ARGUMENTS) != NULL)
        {
            if (UtilityBase != NULL)
            {
                for ( wbArg = SHArg(ARGUMENTS) ; *wbArg ; wbArg++, nbArgs++ )
                {
                    D(bug("[Open] wbArg[%d] %s\n", nbArgs, *wbArg));
                }
                argsTagList = AllocateTagItems(nbArgs + 1);
                for ( wbArg = SHArg(ARGUMENTS) ; *wbArg ; wbArg++, i++ )
                {
                    argsTagList[i].ti_Tag  = WBOPENA_ArgName;
                    argsTagList[i].ti_Data = (IPTR) *wbArg;
                    D(bug("[Open] argsTagList[%d]: %s\n", i, argsTagList[i].ti_Data));
                }
                argsTagList[i].ti_Tag = TAG_DONE;
                D(bug("[Open] i = %d, nbArgs = %d\n", i, nbArgs));
                success = OpenWorkbenchObjectA(SHArg(NAME), argsTagList);
                FreeTagItems(argsTagList);
            }
            else
            {
                PutStr("ERROR: Could not open utility.library v41+.\n");
            }
        }
        else
        {
            D(bug("[Open] SHArg(ARGUMENTS) == NULL\n"));
            success = OpenWorkbenchObject(SHArg(NAME), TAG_DONE);
        }
        if (UtilityBase != NULL)
            CloseLibrary(UtilityBase);
        CloseLibrary(WorkbenchBase);
    }
    else
    {
        PutStr("ERROR: Could not open workbench.library v44+.\n");
    }
    
    return success ? RETURN_OK : RETURN_FAIL;
    
    AROS_SHCOMMAND_EXIT
}
