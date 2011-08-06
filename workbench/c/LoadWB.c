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

const TEXT version[] = "$VER: LoadWB 42.1 (29.7.2011)";

int __nocommandline = 1;

int main(void)
{
    struct WorkbenchBase *WorkbenchBase = TaggedOpenLibrary(TAGGEDOPEN_WORKBENCH);
    if (WorkbenchBase) {
    	StartWorkbench(0, NULL);
    	CloseLibrary((struct Library*)WorkbenchBase);
    }
    return 0;
}
