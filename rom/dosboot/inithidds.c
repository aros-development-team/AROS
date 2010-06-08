/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Code that loads and initializes necessary HIDDs.
    Lang: english
*/

#define DEBUG 0

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <workbench/icon.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <stdlib.h>
#include <string.h>

#include "dosboot_intern.h"

#define MONITORS_DIR "DEVS:Monitors"

/************************************************************************/

/* This code does almost the same thing as C:LoadMonDrvs does on other systems.
   However, additionally we support priority-based sorting for display drivers.
   This is needed in order to make monitor ID assignment more predictable */

static BYTE checkIcon(STRPTR name, struct Library *IconBase)
{
    LONG pri = 0;
    struct DiskObject *dobj = GetDiskObject(name);

    if (dobj == NULL)
	return 0;

    if ((dobj->do_Type == WBTOOL) || (dobj->do_Type == WBPROJECT))
    {
	const STRPTR *toolarray = (const STRPTR *)dobj->do_ToolTypes;
	STRPTR s;

	if ((s = FindToolType(toolarray, "STARTPRI")))
	{
	    pri = atol(s);
	    if (pri < -128)
	        pri = -128;
	    else if (pri > 127)
	        pri = 127;
	}
	FreeDiskObject(dobj);
    }
    return pri;
}

static BOOL findMonitors(struct List *monitorsList, struct DosLibrary *DOSBase, struct Library *IconBase, APTR poolmem)
{
    BOOL retvalue = TRUE;
    LONG error;
    struct AnchorPath *ap = AllocPooled(poolmem, sizeof(struct AnchorPath));

    if (ap)
    {
	error = MatchFirst("~(#?.info)", ap);
	while (!error)
	{
	    struct Node *newnode = AllocPooled(poolmem, sizeof (struct Node));

	    if (newnode == NULL) {
		retvalue = FALSE;
		goto exit;
	    }
	    newnode->ln_Name = AllocPooled(poolmem, strlen(ap->ap_Info.fib_FileName) + 1);
	    if (newnode->ln_Name == NULL) {
		retvalue = FALSE;
		goto exit;
	    }

	    strcpy(newnode->ln_Name, ap->ap_Info.fib_FileName);
	    if (IconBase)
	        newnode->ln_Pri = checkIcon(ap->ap_Info.fib_FileName, IconBase);
	    else
	        newnode->ln_Pri = 0;
	    Enqueue(monitorsList, newnode);

	    error = MatchNext(ap);
	}
	if (error != ERROR_NO_MORE_ENTRIES)
	{
	    retvalue = FALSE;
	    goto exit;
	}
	MatchEnd(ap);
    }
    else
	retvalue = FALSE;
exit:
    return retvalue;
}

static void loadMonitors(struct List *monitorsList, struct DosLibrary *DOSBase)
{
    struct Node *node;

    D(bug("[DOSBoot] Loading monitor drivers...\n"));
    D(bug(" Pri Name\n"));

    ForeachNode(monitorsList, node)
    {
	D(bug("%4d %s\n", node->ln_Pri, node->ln_Name));
	Execute(node->ln_Name, NULL, NULL);
    }

    D(bug("--------------------------\n"));
}

BOOL __dosboot_InitHidds(struct DosLibrary *dosBase)
{
    APTR pool;
    struct Library *IconBase;
    BPTR dir, olddir;
    BOOL res = TRUE;

    dir = Lock(MONITORS_DIR, SHARED_LOCK);
    D(bug("[DOSBoot] Monitors directory 0x%p\n", dir));
    if (dir) {
        olddir = CurrentDir(dir);

        pool = CreatePool(MEMF_ANY, sizeof(struct Node) * 10, sizeof(struct Node) * 5);
        if (pool) {
	    struct List MonitorsList;

	    NewList(&MonitorsList);
	    IconBase = OpenLibrary("icon.library", 0);

            findMonitors(&MonitorsList, dosBase, IconBase, pool);
            loadMonitors(&MonitorsList, dosBase);

	    if (IconBase)
		CloseLibrary(IconBase);
	    DeletePool(pool);
	} else
	    res = FALSE;

	CurrentDir(olddir);
	UnLock(dir);
    }
    return res;
}
