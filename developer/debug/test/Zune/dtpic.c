/*
    Copyright © 2002-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <dos/dos.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>

struct Library *MUIMasterBase;

Object *app;

int main(void)
{
    Object *wnd;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "dtpic test",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
    	    	Child, DtpicObject, MUIA_Dtpic_Name, "SYS:System/Images/AROS.png", End,
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
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }

    CloseLibrary(MUIMasterBase);
    
    return 0;
}

