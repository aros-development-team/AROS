/*
    Copyright  1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Backwards compatibility display driver loader.
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

/************************************************************************/

struct MyArgs
{
    STRPTR hidd;
    STRPTR lib;
};

extern struct WBStartup *WBenchMsg;

int __nocommandline = 1;

int main(void)
{
    BPTR olddir = BNULL;
    STRPTR myname;
    struct DiskObject *icon;
    struct RDArgs *rdargs = NULL;
    int res = RETURN_OK;
    struct MyArgs args = {NULL};

    if (WBenchMsg) {
        olddir = CurrentDir(WBenchMsg->sm_ArgList[0].wa_Lock);
	myname = WBenchMsg->sm_ArgList[0].wa_Name;
    } else {
	struct Process *me = (struct Process *)FindTask(NULL);
    
	if (me->pr_CLI) {
            struct CommandLineInterface *cli = BADDR(me->pr_CLI);
	
	    myname = AROS_BSTR_ADDR(cli->cli_CommandName);
	} else
	    myname = me->pr_Task.tc_Node.ln_Name;
    }   
    D(Printf("Command name: %s\n", myname));

    icon = GetDiskObject(myname);
    D(Printf("Icon 0x%p\n", icon));

    if (icon) {
	args.hidd = FindToolType(icon->do_ToolTypes, "CLASS");
        args.lib = FindToolType(icon->do_ToolTypes, "LIBRARY");
    }

    if (!WBenchMsg) {
        rdargs = ReadArgs("CLASS=HIDD/A,LIBRARY=LIB", (IPTR *)&args, NULL);
	D(Printf("RDArgs 0x%p\n", rdargs));
    }
 
    D(Printf("CLASS=%s, LIBRARY=%s\n", args.hidd ? args.hidd : "<none>",
	     args.lib ? args.lib : "<none>"));
 
    if (args.hidd)
    {
        OOP_Class *cl;
	struct Library *gfxlib = NULL;

	cl = OOP_FindClass(args.hidd);
	if (!cl)
	{
	    if (args.lib)
	    {
		gfxlib = OpenLibrary(args.lib, 0);
	        if (!gfxlib)
		    res = RETURN_ERROR;
	    }
	    
	    if (res == RETURN_OK)
	    {
		cl = OOP_FindClass(args.hidd);
		if (cl)
		{
		    if (AddDisplayDriverA(cl, NULL, NULL))
		    {
			res = RETURN_FAIL;
		    }
		}
		else
		    res = RETURN_ERROR;
		
		if ((res != RETURN_OK) && gfxlib)
		    CloseLibrary(gfxlib);
	    }
	}
    } else
	res = RETURN_ERROR;

    if (rdargs)
        FreeArgs(rdargs);
    if (icon)
        FreeDiskObject(icon);
    if (olddir)
        CurrentDir(olddir);

    return res;
}
