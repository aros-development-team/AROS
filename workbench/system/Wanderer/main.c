/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#define DEBUG 1

#include <exec/types.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <dos/dostags.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include <string.h>

#include <libraries/mui.h>
#include <aros/debug.h>

#include "iconwindow.h"
#include "wanderer.h"
#include "support.h"

/* Our global variables */

Object *app;
Object *root_iconwnd;
Object *root_menustrip;

STRPTR rootBG;
STRPTR dirsBG;

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

VOID LoadPrefs(VOID)
{
    BPTR fh;
    
    if ((fh = Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE)) != NULL)
    {
        STRPTR buffer = NULL;
        LONG   size;
        
        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_BEGINNING) + 2;
        
        if ((buffer = AllocVec(size, MEMF_ANY)) != NULL)
        {
            if (!ReadLine(fh, buffer, size)) goto end;
            if (rootBG == NULL)
            {
                rootBG = StrDup(buffer);
            }
            else if (strcmp(rootBG, buffer) != 0)
            {
                FreeVec(rootBG);
                rootBG = StrDup(buffer);
                
                if (rootBG != NULL)
                {
                    SET
                    (
                        (Object *) XGET(root_iconwnd, MUIA_IconWindow_IconList),
                        MUIA_Background, (IPTR) rootBG
                    );
                }
            }
            
            if (!ReadLine(fh, buffer, size)) goto end;
            if (dirsBG == NULL)
            {
                dirsBG = StrDup(buffer);
            }
            else if (strcmp(dirsBG, buffer) != 0)
            {
                FreeVec(dirsBG);
                dirsBG = StrDup(buffer);
                
                if (dirsBG != NULL)
                {
                    Object *cstate = (Object*)(((struct List*)XGET(_app(root_iconwnd), MUIA_Application_WindowList))->lh_Head);
                    Object *child;
        
                    while ((child = NextObject(&cstate)))
                    {
                        if (child != root_iconwnd && XGET(child, MUIA_UserData))
                        {
                            SET
                            (
                                (Object *) XGET(child, MUIA_IconWindow_IconList),
                                MUIA_Background, (IPTR) dirsBG
                            );
                        }
                    }
                }
            }
            
end:        FreeVec(buffer);
        }
        
        Close(fh);
    }
}

VOID FreePrefs()
{
    if (rootBG != NULL) FreeVec(rootBG);
    if (dirsBG != NULL) FreeVec(dirsBG);
}

int main(void)
{
    LoadPrefs();

    app = WandererObject,
    End;

    if (app != NULL)
    {
	DoDetach();
	DoMethod(app, MUIM_Application_Execute);
        
	MUI_DisposeObject(app);
    }

    FreePrefs();
    
    return 0;
}
