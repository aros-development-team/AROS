/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbench replacement
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "workbook.h"

int main(int argc, char **argv)
{
	struct WorkbookBase *wb;
	int rc = RETURN_ERROR;

	wb = NULL;

	wb = AllocVec(sizeof(*wb), MEMF_CLEAR);
	if (!wb)
		goto error;

	DOSBase = OpenLibrary("dos.library", 0);
	if (DOSBase == NULL)
		goto error;

	IntuitionBase = OpenLibrary("intuition.library",0);
	if (IntuitionBase == NULL)
		goto error;

	UtilityBase = OpenLibrary("utility.library",0);
	if (UtilityBase == NULL)
	    goto error;

	GadToolsBase = OpenLibrary("gadtools.library",0);
	if (GadToolsBase == NULL)
	    goto error;

	/* Version 44 or later for DrawIconStateA */
	IconBase = OpenLibrary("icon.library",44);
	if (IconBase == NULL)
	    goto error;

	/* Version 44 or later for OpenWorkbenchObject */
	WorkbenchBase = OpenLibrary("workbench.library",44);
	if (WorkbenchBase == NULL)
	    goto error;

	GfxBase = OpenLibrary("graphics.library",0);
	if (GfxBase == NULL)
	    goto error;

	rc = WB_Main(wb);

error:
	if (wb) {
	    if (GfxBase)
	    	CloseLibrary(GfxBase);

	    if (WorkbenchBase)
	    	CloseLibrary(WorkbenchBase);

	    if (IconBase)
	    	CloseLibrary(IconBase);

	    if (GadToolsBase)
	    	CloseLibrary(GadToolsBase);

	    if (UtilityBase)
	    	CloseLibrary(UtilityBase);

	    if (IntuitionBase)
	    	CloseLibrary(IntuitionBase);

	    if (DOSBase)
	    	CloseLibrary(DOSBase);

	    FreeVec(wb);
	}

	return rc;
}

void snoop(Class *cl, Object *obj, Msg msg)
{
    //struct WorkbookBase *wb = (APTR)cl->cl_UserData;

}
