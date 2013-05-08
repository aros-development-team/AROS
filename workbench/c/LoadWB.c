/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Load the default Workbench
    Lang: English
*/

#define  DEBUG  0
#include <aros/debug.h>

#include <exec/types.h>
#include <proto/exec.h>
#include <workbench/workbench.h>
#include <proto/workbench.h>

#include <aros/shcommands.h>

const TEXT version[] = "$VER: LoadWB 42.2 (7.9.1999)";

/* Very minimal C:LoadWB */
AROS_SH0H(LoadWB, 42.2, "Load the default Workbench")
{
    AROS_SHCOMMAND_INIT

    struct Library *WorkbenchBase = OpenLibrary("workbench.library", 0);
    if (WorkbenchBase) {
    	StartWorkbench(0, NULL);
    	CloseLibrary(WorkbenchBase);
    }
    return 0;

    AROS_SHCOMMAND_EXIT
}
