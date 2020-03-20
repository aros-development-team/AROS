/*
    Copyright © 2002-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <dos/dos.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/wb.h>
#include <proto/dos.h>

#include "locale.h"
#include "wanderer.h"

/* global variables */
Object         *_WandererIntern_AppObj = NULL;
Class          *_WandererIntern_CLASS = NULL;

/* Don't output errors to the console, open requesters instead */
int                  __forceerrorrequester = 1;

///main()
int main(void)
{
    LONG             retval = RETURN_ERROR;

    /* WB programs have NULL pr_ConsoleTask */
    SetConsoleTask(NULL);

    D(
        struct Task *me = (struct Task *)FindTask(NULL);

        bug("[Wanderer] %s: This Task @ %p \n", __PRETTY_FUNCTION__, me);
        bug("[Wanderer] %s: SPLower @ %p \n", __PRETTY_FUNCTION__, me->tc_SPLower);
        bug("[Wanderer] %s: SPUpper @ %p \n", __PRETTY_FUNCTION__, me->tc_SPUpper);
    )

    D(bug("[Wanderer] %s: Wanderer Initialising .. \n", __PRETTY_FUNCTION__));
    if ((_WandererIntern_AppObj = NewObject(Wanderer_CLASS->mcc_Class, NULL, TAG_DONE)) != NULL)
    {
        D(bug("[Wanderer] %s: Launching WBStartup items .. \n", __PRETTY_FUNCTION__));
        OpenWorkbenchObject("Wanderer:Tools/ExecuteStartup", TAG_DONE);

        D(bug("[Wanderer] %s: Handing control over to Zune .. \n", __PRETTY_FUNCTION__));
        retval = DoMethod(_WandererIntern_AppObj, MUIM_Application_Execute);

        D(bug("[Wanderer] %s: Returned from Zune's control .. \n", __PRETTY_FUNCTION__));
        MUI_DisposeObject(_WandererIntern_AppObj);
    }

    D(bug("[Wanderer] %s: Exiting .. \n", __PRETTY_FUNCTION__));

    return retval;
}
///
