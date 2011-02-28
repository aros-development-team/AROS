/*
    Copyright © 2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <dos/dosextens.h>
#include <libraries/mui.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>


struct Library *MUIMasterBase;

Object *app;

int __nocommandline = 1;

Object *MakeFontsGroup(SIPTR FontAttr, STRPTR title)
{
    return VGroup,
	GroupFrameT(title),
	MUIA_Font, FontAttr,
	Child, TextObject, MUIA_Text_Contents, "No font specification", End,
	Child, TextObject, MUIA_Text_Contents, "Normal font specification", MUIA_Font, MUIV_Font_Normal, End,
	Child, TextObject, MUIA_Text_Contents, "Tiny font specification", MUIA_Font, MUIV_Font_Tiny, End,
	Child, TextObject, MUIA_Text_Contents, "Fixed font specification", MUIA_Font, MUIV_Font_Fixed, End,
	Child, TextObject, MUIA_Text_Contents, "Title font specification", MUIA_Font, MUIV_Font_Title, End,
	Child, TextObject, MUIA_Text_Contents, "Big font specification", MUIA_Font, MUIV_Font_Big, End,
	Child, TextObject, MUIA_Text_Contents, "Button font specification", MUIA_Font, MUIV_Font_Button, End,
	Child, TextObject, MUIA_Text_Contents, "Knob font specification", MUIA_Font, MUIV_Font_Knob, End,
	End;
}

int main(void)
{
    Object *wnd;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "Font specification test",
	    MUIA_Window_Activate, TRUE,
    	    WindowContents, HGroup,
		Child, MakeFontsGroup(MUIV_Font_Inherit, "Group with default font"),
		Child, MakeFontsGroup(MUIV_Font_Fixed, "Group with fixed font"),
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod
        (
            wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR) app, 
            2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
        );
	
	set(wnd,MUIA_Window_Open,TRUE);

	while (DoMethod(app, MUIM_Application_NewInput, (IPTR) &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C);
		if (sigs & SIGBREAKF_CTRL_C) break;
	    }
	}

	MUI_DisposeObject(app);
    }

    CloseLibrary(MUIMasterBase);
    
    return 0;
}

