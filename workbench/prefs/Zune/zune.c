/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <libraries/mui.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "zunestuff.h"

struct Library *MUIMasterBase;

#ifndef _AROS

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int open_muimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = (void*)OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = (void*)OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = (void*)OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = (void*)OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    MUIMasterBase_instance.gadtoolsbase = OpenLibrary("gadtools.library",37);
    __zune_prefs_init(&__zprefs);
    InitSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    return 1;
}

void close_muimaster(void)
{
}

#else

int open_muimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void close_muimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif



/****************************************************************
 Open needed libraries
*****************************************************************/
int open_libs(void)
{
    if (open_muimaster())
    {
	return 1;
    }
}

/****************************************************************
 Close opened libraries
*****************************************************************/
void close_libs(void)
{
    close_muimaster();
}

static Object *app;
static Object *open_menuitem;
static Object *append_menuitem;
static Object *saveas_menuitem;
static Object *aboutzune_menuitem;
static Object *quit_menuitem;
static Object *main_wnd;

/****************************************************************
 Allocalte resources for gui
*****************************************************************/
int init_gui(void)
{
    Object *save_button;
    Object *use_button;
    Object *test_button;
    Object *cancel_button;

    app = ApplicationObject,
	MUIA_Application_Menustrip, MenuitemObject,
	    MUIA_Family_Child, MenuitemObject,
	    	MUIA_Menuitem_Title, "Project",
	    	MUIA_Family_Child, open_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Open...", MUIA_Menuitem_Shortcut, "O", End,
	    	MUIA_Family_Child, append_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Append...", End,
	    	MUIA_Family_Child, saveas_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Save As...", MUIA_Menuitem_Shortcut, "A", End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, ~0, End,
	    	MUIA_Family_Child, aboutzune_menuitem = MenuitemObject, MUIA_Menuitem_Title, "About Zune...", MUIA_Menuitem_Shortcut, "?", End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, ~0, End,
	    	MUIA_Family_Child, quit_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Quit", MUIA_Menuitem_Shortcut, "Q", End,
	    	End,
	    End,
    	SubWindow, main_wnd = WindowObject,
    	    MUIA_Window_Title, "Zune - Preferences",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, HGroup,
    	    	Child, save_button = MakeButton("Save"),
    	    	Child, use_button = MakeButton("Use"),
    	    	Child, test_button = MakeButton("Test"),
    	    	Child, cancel_button = MakeButton("Cancel"),
		End,
    	    End,
    	End;

    if (app)
    {
	DoMethod(main_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(quit_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	return 1;
    }
    return 0;
}

/****************************************************************
 Deallocates all gui resources
*****************************************************************/
void deinit_gui(void)
{
    if (app) MUI_DisposeObject(app);
}

/****************************************************************
 The message loop
*****************************************************************/
void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
    {
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
	    if (sigs & SIGBREAKF_CTRL_C) break;
	    if (sigs & SIGBREAKF_CTRL_D) break;
	}
    }
}

/****************************************************************
 The main entry point
*****************************************************************/
void main(void)
{
    if (open_libs())
    {
    	if (init_gui())
    	{
    	    set(main_wnd, MUIA_Window_Open, TRUE);
    	    if (xget(main_wnd,MUIA_Window_Open))
	    {
		loop();
	    }
	    deinit_gui();
    	}
	close_libs();
    }
}
