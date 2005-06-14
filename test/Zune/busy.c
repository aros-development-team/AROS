/*
    Copyright © 2002, The AROS Development Team.
    All rights reserved.

    $Id: crawling.c 19971 2003-10-16 15:47:01Z stegerg $
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
#include <MUI/Busy_mcc.h>

/* the following should go in a single include file which then only
** constits of the public constants and members. Actually this is easiey
*/

#include <libraries/mui.h>

struct Library *MUIMasterBase;

Object *app;

int main(void)
{
    Object *wnd, *slider, *busy;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "busy",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
	    	Child, slider = SliderObject, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 250, MUIA_Slider_Horiz, TRUE, End,
    	    	Child, busy = BusyBar,
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

    	DoMethod(slider, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime,
	    	 (IPTR)busy, 3, MUIM_Set, MUIA_Busy_Speed, MUIV_TriggerValue);
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

