/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: WBRun CLI command
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

AROS_SH6H
(
    WBRun, 44.1,
        "Open/launch a workbench object as if its icon were double-clicked",
    AROS_SHAH(STRPTR, , PROG, /A, "", "\tName of the object to be opened."),
    AROS_SHAH(STRPTR *, , ARGS, /M, NULL,
        "\tList of parameters (Workbench arguments) to pass to\n"
        "\t\tthe tool/project to be launched."),
    AROS_SHAH(STRPTR, , SHOW, /K, "",
        "\tControls whether files and drawers without icons will be\n"
        "\t\tshown (if a drawer is opened). Possible values are ICONS\n"
        "\t\tor ALL. [unimplemented]"),
    AROS_SHAH(STRPTR, , VIEWBY, /K, "",
        "Viewing mode for a drawer. Possible values are ICON, NAME,\n"
        "\t\tDATE or SIZE. [unimplemented]"),
    AROS_SHAH(IPTR, , DELAY, /K/N, "",
        "Seconds to wait before opening the object. [unimplemented]"),
    AROS_SHAH(BOOL, , QUIET, /S, "",
        "\tDo not show requesters if object is unavailable.\n"
        "\t\t[unimplemented]")
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
        if (SHArg(ARGS) != NULL)
        {
            if (UtilityBase != NULL)
            {
                for ( wbArg = SHArg(ARGS) ; *wbArg ; wbArg++, nbArgs++ )
                {
                    D(bug("[WBRun] wbArg[%d] %s\n", nbArgs, *wbArg));
                }
                argsTagList = AllocateTagItems(nbArgs + 1);
                for ( wbArg = SHArg(ARGS) ; *wbArg ; wbArg++, i++ )
                {
                    argsTagList[i].ti_Tag  = WBOPENA_ArgName;
                    argsTagList[i].ti_Data = (IPTR) *wbArg;
                    D(bug("[WBRun] argsTagList[%d]: %s\n", i, argsTagList[i].ti_Data));
                }
                argsTagList[i].ti_Tag = TAG_DONE;
                D(bug("[WBRun] i = %d, nbArgs = %d\n", i, nbArgs));
                success = OpenWorkbenchObjectA(SHArg(PROG), argsTagList);
                FreeTagItems(argsTagList);
            }
            else
            {
                PutStr("ERROR: Could not open utility.library v41+.\n");
            }
        }
        else
        {
            D(bug("[WBRun] SHArg(ARGS) == NULL\n"));
            success = OpenWorkbenchObject(SHArg(PROG), TAG_DONE);
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
