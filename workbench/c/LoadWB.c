/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Load the default Workbench
*/

#define  DEBUG  0
#include <aros/debug.h>

#include <exec/types.h>
#include <proto/exec.h>
#include <workbench/workbench.h>
#include <proto/workbench.h>

#include <aros/shcommands.h>

const TEXT version[] = "$VER: LoadWB 42.3 (27.09.2026)";

/* Very minimal C:LoadWB */
AROS_SH0H(LoadWB, 42.3, "Load the default Workbench")
{
    AROS_SHCOMMAND_INIT

    struct Library *WorkbenchBase = OpenLibrary("workbench.library", 0);
    if (!WorkbenchBase)
        return RETURN_FAIL;

    StartWorkbench(0, NULL);
    CloseLibrary(WorkbenchBase);

    return RETURN_OK;

    AROS_SHCOMMAND_EXIT
}
