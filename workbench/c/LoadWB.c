/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: LoadWB.c $

    Desc:
    Lang: English
*/

#define  DEBUG  0
#include <aros/debug.h>

#include <exec/types.h>
#include <proto/exec.h>
#include <workbench/workbench.h>
#include <proto/workbench.h>

#include <strings.h>

/* Very minimal C:LoadWB */

const TEXT version[] = "$VER: LoadWB 42.2 (16.6.2012)";

int __startup _main(void)
{
    struct Library *WorkbenchBase = OpenLibrary("workbench.library", 0);
    if (WorkbenchBase) {
    	StartWorkbench(0, NULL);
    	CloseLibrary(WorkbenchBase);
    }
    return 0;
}
