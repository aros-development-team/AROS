/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>

//#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#ifdef __AROS__
#include <libraries/mui.h>
#endif
#include <clib/alib_protos.h>
#include <stdio.h>

struct Library       *MUIMasterBase;

#ifndef __AROS__

#include <mui.h>
#undef SysBase

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int openmuimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    __zune_prefs_init(&__zprefs);

    return 1;
}

void closemuimaster(void)
{
}

#else

int openmuimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void closemuimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif

int main (int argc, char **argv)
{
    Object *app;
    Object *mainWin;
    Object *pages;
    Object *stepb[2];
    Object *b[3];
    int i;

    if (!openmuimaster()) return 20;

    app = ApplicationObject,
	SubWindow, mainWin = WindowObject,
	    MUIA_Window_Title, "Pages",
	    WindowContents, VGroup,
                Child, VGroup,
   		    GroupFrame,
		    Child, HGroup,
			Child, stepb[0] = RectangleObject,
				MUIA_Rectangle_BarTitle, "prev",
	                        MUIA_CycleChain, TRUE,
				MUIA_InnerLeft, 20,
				MUIA_FixHeight, 20,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, stepb[1] = RectangleObject,
				MUIA_Rectangle_BarTitle, "next",
	                        MUIA_CycleChain, TRUE,
				MUIA_InnerLeft, 20,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
		    End,

	            Child, MUI_MakeObject(MUIO_HBar, 4),
		    Child, HGroup,
			Child, b[0] = RectangleObject,
				MUIA_Rectangle_BarTitle, "1",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, b[1] = RectangleObject,
				MUIA_Rectangle_BarTitle, "2",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
			Child, b[2] = RectangleObject,
				MUIA_Rectangle_BarTitle, "3",
	                        MUIA_CycleChain, TRUE,
				MUIA_InputMode, MUIV_InputMode_RelVerify,
				MUIA_FixHeight, 20,
				MUIA_Background, MUII_ButtonBack,
				ButtonFrame,
			End,
		    End,
                End,
	        Child, pages = HGroup,
	            MUIA_Group_PageMode, TRUE,
	            InputListFrame,
	            MUIA_Background, MUII_PageBack,
	            Child, RectangleObject,
	                    MUIA_FixWidth, 50,
	                    MUIA_Background, MUII_SHADOW,
                            MUIA_Rectangle_BarTitle, "Rect1",
	            End,
                    Child, RectangleObject,
	                    MUIA_FixWidth, 30,
                            MUIA_FixHeight, 30,
	                    MUIA_Background, MUII_SHINE,
                            MUIA_Rectangle_BarTitle, "Rect2",
                    End,
                    Child, RectangleObject,
                            MUIA_FixHeight, 50,
	                    MUIA_Background, MUII_FILL,
                            MUIA_Rectangle_BarTitle, "Rect3",
                    End,
                End,
            End,
        End,
    End;

    if (!app)
    {
	fprintf(stderr, "can't create application object.\n");
	goto error;
    }

    DoMethod(mainWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	     (IPTR)app, 2,
	     MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    DoMethod(stepb[0], MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
	     (IPTR)pages, 3, MUIM_Set,
	     MUIA_Group_ActivePage, MUIV_Group_ActivePage_Prev);

    DoMethod(stepb[1], MUIM_Notify, MUIA_Timer, MUIV_EveryTime,
	     (IPTR)pages, 3, MUIM_Set,
	     MUIA_Group_ActivePage, MUIV_Group_ActivePage_Next);

    for (i = 0; i < 3; i++)
    {
	DoMethod(b[i], MUIM_Notify, MUIA_Pressed, FALSE,
		 (IPTR)pages, 3, MUIM_Set, MUIA_Group_ActivePage, i);
    }
    /*
     * Open window and ALWAYS check.
     */
    set(mainWin, MUIA_Window_Open, TRUE);
    if (!xget(mainWin, MUIA_Window_Open))
    {
	MUI_DisposeObject(app);
	fprintf(stderr, "can't open main window.\n");
	goto error;
    }

    {
	ULONG sigs = 0;

	while (DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
	       != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
	        sigs = Wait(sigs | SIGBREAKF_CTRL_C);
	        if (sigs & SIGBREAKF_CTRL_C) break;
	    }
	}
    }
    
    set(mainWin, MUIA_Window_Open, FALSE);
    MUI_DisposeObject(app);

error:

    closemuimaster();

    return 0;
}

/*** EOF ***/
