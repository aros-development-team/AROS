/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Disk-resident part of GDI display driver
    Lang: english
*/

#include <aros/debug.h>
#include <dos/dosextens.h>
#include <oop/oop.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/oop.h>
#include <proto/icon.h>

#include <stdlib.h>

#include "gdi_class.h"

/* Minimum required library version */
#define GDI_VERSION 42

/************************************************************************/

/*
 * This program actually just creates additional GDI displays.
 * It assumes that the driver itself is placed in kickstart in the form
 * of library
 */

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

/* This function uses library open count as displays count */
static ULONG AddDisplays(ULONG num)
{
    struct GDIBase *GDIBase;
    OOP_Class *gfxclass;
    ULONG old;
    ULONG i;

    D(bug("[GDI] Making %u displays\n", num));
    /* First query current displays count */
    GDIBase = (struct GDIBase *)OpenLibrary(GDI_LIBNAME, GDI_VERSION);
    if (!GDIBase)
        return 0;

    gfxclass = GDIBase->gfxclass;
    old      = GDIBase->displaynum - 1;

    CloseLibrary(&GDIBase->library);
    D(bug("[GDI] Current displays count: %u\n", old));

    /* Add displays if needed */
    for (i = old; i < num; i++)
    {
	ULONG err = AddDisplayDriverA(gfxclass, NULL, NULL);

	if (err)
	{
	    /* Abort if driver setup failed */
	    D(bug("[GDI] Failed to add display object, error %u\n", err));
	    break;
	}
    }

    return i;
}

int main(void)
{
    BPTR olddir = NULL;
    STRPTR myname;
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    int res = RETURN_OK;
    IPTR displays = 1;

    if (WBenchMsg) {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
	myname = WBenchMsg->sm_ArgList[0].wa_Name;
    } else {
	struct Process *me = (struct Process *)FindTask(NULL);
    
	if (me->pr_CLI) {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);
	
	    myname = cli->cli_CommandName;
	} else
	    myname = me->pr_Task.tc_Node.ln_Name;
    }
    D(bug("[GDI] Command name: %s\n", myname));

    icon = GetDiskObject(myname);
    D(bug("[GDI] Icon 0x%p\n", icon));

    if (icon) {
        STRPTR str = FindToolType(icon->do_ToolTypes, "DISPLAYS");
        
	displays = atoi(str);
    }

    if (!WBenchMsg) {
        rdargs = ReadArgs("DISPLAYS/N/A", &displays, NULL);
	D(bug("[GDI] RDArgs 0x%p\n", rdargs));
    }
 
    AddDisplays(displays);

    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);

    return res;
}
