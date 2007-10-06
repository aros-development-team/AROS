/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <libraries/mui.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <dos/dos.h>

#include "locale.h"
#include "wanderer.h"

/* global variables */
Object 				 *_WandererIntern_AppObj = NULL;
Class 				 *_WandererIntern_CLASS = NULL;

/* Don't output errors to the console, open requesters instead */
int                  __forceerrorrequester = 1;

int main(void)
{
    LONG             retval = RETURN_ERROR;

D(bug("[Wanderer.EXE] Wanderer Initialising .. \n"));

    OpenWorkbenchObject("Wanderer:Tools/ExecuteStartup", 0, 0);
	
    if ((_WandererIntern_AppObj = WandererObject, End) != NULL)
    {
D(bug("[Wanderer.EXE] Handing control over to Zune .. \n"));
		retval = DoMethod(_WandererIntern_AppObj, MUIM_Application_Execute);
D(bug("[Wanderer.EXE] Returned from Zune's control .. \n"));
		MUI_DisposeObject(_WandererIntern_AppObj);
    }

D(bug("[Wanderer.EXE] Exiting .. \n"));

    return retval;
}
