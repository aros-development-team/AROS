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

extern struct Hook hook_action;

VOID DoAllMenuNotifies(Object *strip, char *path);
VOID LoadPrefs(VOID);

AROS_UFP3(void, hook_func_action,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(Object *, obj, A2),
    AROS_UFPA(struct IconWindow_ActionMsg *, msg, A1));

AROS_UFP3
(
    void, hook_func_standard,
    AROS_UFPA(struct Hook *, h, A0),
    AROS_UFPA(void *, dummy, A2),
    AROS_UFPA(void **, funcptr, A1)
);

/**************************************************************************
 This is the standard_hook for easy MUIM_CallHook callbacks
 It is initialized at the very beginning of the main program
**************************************************************************/
struct Hook hook_standard;


AROS_UFH3
(
    void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);

    if (func) func((ULONG *)(funcptr + 1));
}




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
extern struct Hook hook_action;

/**************************************************************************
 Our main entry
**************************************************************************/
int main(void)
{
    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    hook_action.h_Entry = (HOOKFUNC)hook_func_action;

    LoadPrefs();

    app = WandererObject,
	MUIA_Application_Title, (IPTR) "Wanderer",
	MUIA_Application_Base, (IPTR) "WANDERER",
	MUIA_Application_Version, (IPTR) "$VER: Wanderer 0.1 (10.12.02)",
	MUIA_Application_Description, (IPTR) "The AROS filesystem GUI",
	MUIA_Application_SingleTask, TRUE,
    	SubWindow, root_iconwnd = IconWindowObject,
	    MUIA_UserData, 1,
	    //MUIA_Window_Menustrip, root_menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
	    MUIA_Window_ScreenTitle, NULL,//GetScreenTitle(),
            MUIA_IconWindow_IsRoot, TRUE,
	    MUIA_IconWindow_IsBackdrop, TRUE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	//DoMethod(root_iconwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 3, MUIM_CallHook, &hook_standard, wanderer_quit);

	/* If "Execute Command" entry is clicked open the execute window */
	DoAllMenuNotifies(root_menustrip,"RAM:");

	/* And now open it */
	DoMethod(root_iconwnd, MUIM_IconWindow_Open);

	DoDetach();

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }

    FreePrefs();
    
    return 0;
}
