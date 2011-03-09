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

#include "workbook.h"
#include "classes.h"

int WB_Main(struct WorkbookBase *wb)
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
