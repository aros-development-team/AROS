/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <dos/dos.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "wanderer.h"
#include "prefs.h"

/* global variables */
Object *app;
Object *root_iconwnd;
Object *root_menustrip;

LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Wanderer";

VOID DoDetach(VOID)
{
    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal
        (
            (struct Task *) __detacher_process, 
            __detacher_must_wait_for_signal
        );
    }
}

int main(void)
{
    LoadPrefs();

    app = WandererObject, End;
    if (app != NULL)
    {
	DoDetach();
	DoMethod(app, MUIM_Application_Execute);
        
	MUI_DisposeObject(app);
    }

    FreePrefs();
    
    return 0;
}
