/*
    Copyright © 2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Workbench replacement
    Lang: english
*/
#include <stdio.h>

#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/icon.h>
#include <proto/workbench.h>

#include "workbook_intern.h"
#include "classes.h"

/* Allocate classes and run the main app */
static int WB_Main(struct WorkbookBase *wb)
{
    int rc = RETURN_OK;

    wb->wb_WBApp = WBApp_MakeClass(wb);
    if (!wb->wb_WBApp)
    	goto exit;

    wb->wb_WBWindow = WBWindow_MakeClass(wb);
    if (!wb->wb_WBWindow)
    	goto exit;

    wb->wb_WBVirtual = WBVirtual_MakeClass(wb);
    if (!wb->wb_WBVirtual)
    	goto exit;

    wb->wb_WBSet    = WBSet_MakeClass(wb);
    if (!wb->wb_WBSet)
    	goto exit;

    wb->wb_WBIcon   = WBIcon_MakeClass(wb);
    if (!wb->wb_WBIcon)
    	goto exit;

    wb->wb_App = NewObject(WBApp, NULL, TAG_END);
    if (wb->wb_App) { 
    	DoMethod(wb->wb_App, WBAM_WORKBENCH, 0);
    	DisposeObject(wb->wb_App);
    	rc = 0;
    }

exit:
    if (wb->wb_WBIcon)
    	FreeClass(wb->wb_WBIcon);
    if (wb->wb_WBSet)
    	FreeClass(wb->wb_WBSet);
    if (wb->wb_WBVirtual)
    	FreeClass(wb->wb_WBVirtual);
    if (wb->wb_WBWindow)
    	FreeClass(wb->wb_WBWindow);
    if (wb->wb_WBApp)
    	FreeClass(wb->wb_WBApp);

    return rc;
}

ULONG WorkbookMain(void)
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

        if (IntuitionBase)
            CloseLibrary(GadToolsBase);

        if (DOSBase)
            CloseLibrary(DOSBase);

        FreeVec(wb);
    }

    return rc;
}
