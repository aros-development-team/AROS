/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <exec/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <clib/alib_protos.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include <libraries/mui.h>

struct Library *MUIMasterBase;

Object *app;

int main(void)
{
    Object *wnd;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "virttest",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
    	    	Child, SimpleButton("hello"),
		Child, ScrollgroupObject,
		    MUIA_Scrollgroup_Contents, HGroupV,
		    	VirtualFrame,
			Child, VGroup,
			    Child, SimpleButton("one"),
			    Child, SimpleButton("two"),
			    Child, SimpleButton("one"),
			    Child, SimpleButton("two"),
			    Child, SimpleButton("one"),
			    Child, SimpleButton("two"),
			    End,
			Child, VGroup,
			    Child, SimpleButton("one"),
			    Child, SimpleButton("two"),
			    Child, SimpleButton("one"),
			    End,
			End,
		    End,
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	set(wnd,MUIA_Window_Open,TRUE);

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

    CloseLibrary(MUIMasterBase);
    
    return 0;
}

