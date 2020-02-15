/* 
    Copyright © 1999, David Le Corfec.
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <dos/dos.h>

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

    if (!openmuimaster()) return 0;

    /*
     * This big call uses macros from mui.h
     * You may also create intermediate objects.
     * Usually you will only create one window, which will be
     * an instance of a custom Window subclass (remember to use
     * subclasses as much as possible !)
     */
    app = ApplicationObject,
        MUIA_Application_Author, "Reez",
	MUIA_Application_Title, "Layout1",
	MUIA_Application_Version, "$VER: Layout1 1.0 (01.08.99)",
        MUIA_Application_Description, "A very simple test program",
	SubWindow, mainWin = WindowObject,
	    MUIA_Window_Title, "My first MUI app !",
	    WindowContents, VGroup,
                Child, HGroup,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_ButtonBack,
                            MUIA_Frame, MUIV_Frame_Button,
                            MUIA_Weight, 60,
	                    MUIA_InnerLeft, 10,
	                    MUIA_InnerRight, 10,
	                    MUIA_InnerTop, 10,
	                    MUIA_InnerBottom, 10,
/*  	                    MUIA_FixWidth, 30, */
                        End,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_FILL,
                            MUIA_Frame, MUIV_Frame_Button,
  	                    MUIA_FixWidth, 42,
                        End,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_FILLSHINE,
                            MUIA_Frame, MUIV_Frame_Button,
                            MUIA_Weight, 40,
                        End,
                End,
                Child, HGroup,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_SHADOWBACK,
                            MUIA_Frame, MUIV_Frame_Button,
  	                    MUIA_FixHeight, 30,
                        End,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_TextBack,
                            MUIA_Frame, MUIV_Frame_Button,
                        End,
	        End,
                Child, HGroup,
	                MUIA_Weight, 50,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_FILLSHINE,
                            MUIA_Frame, MUIV_Frame_Button,
/*  	                    MUIA_FixWidth, 30, */
                        End,
                        Child, RectangleObject,
	                    MUIA_Background, MUII_FILLBACK,
                            MUIA_Frame, MUIV_Frame_Button,
                            MUIA_Weight, 50,
                        End,
	        End,
            End,
        End,
    End;

    if (!app)
    {
	fprintf(stderr, "ERROR: can't create application object.\n");
	goto error;
    }
    printf("created Application object %p\n", app);

    DoMethod(mainWin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	     (IPTR)app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    /*
     * Open window and ALWAYS check.
     */
    set(mainWin, MUIA_Window_Open, TRUE);
    if (!XGET(mainWin, MUIA_Window_Open))
    {
	MUI_DisposeObject(app);
	fprintf(stderr, "ERROR: can't open main window.\n");
	goto error;
    }

/*
** This is the ideal input loop for an object oriented MUI application.
** Everything is encapsulated in classes, no return ids need to be used,
** we just check if the program shall terminate.
** Note that MUIM_Application_NewInput expects sigs to contain the result
** from Wait() (or 0). This makes the input loop significantly faster.
*/
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



#ifdef FOO
void
foo ()
{
    /*
     * Dynamic window adding
     */
    {
	Object *dynWin;

	dynWin = WindowObject,
	    MUIA_Window_Title, "A dynamic window",
	    WindowContents, RectangleObject,
	        MUIA_Background, MUII_SHINE, /* a MUI-fixed color */
	        End,
            End,
	End;

        if (!dynWin)
	{
	    set(mainWin, MUIA_Window_Open, FALSE);
	    MUI_DisposeObject(appli);
	    g_error("can't create dynamic window\n");
	}

	DoMethod(app, OM_ADDMEMBER, dynWin);
	/* Window added to Application window list.
	 * Then you should open it ...
	 */
/*
  set(dynWin, MUIA_Window_Open, TRUE);
  if (!XGET(dynWin, MUIA_Window_Open))
  {
  MUI_DisposeObject(appli);
  set(mainWin, MUIA_Window_Open, FALSE);
  g_error("can't open dynamic window.\n");
  }
       
  set(dynWin, MUIA_Window_Open, FALSE);
*/
	/*
	 * Remove window from application
	 */
	DoMethod(app, OM_REMMEMBER, dynWin);
	/*
	 * If you intend to close a window from one of its methods
	 * (because you subclassed it), use MUIM_Application_PushMethod
	 * to ask another object to destroy you.
	 * Never EVER dispose yourself !!!
	 */
	MUI_DisposeObject(dynWin);
    }
}
#endif

/*** EOF ***/
