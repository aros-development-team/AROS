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

#define DEBUG 1
#include <aros/debug.h>

Object *app;

int main(void)
{
    Object *wnd;
    static char *radio_entries2[] = {"Paris","London",NULL};

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    WindowContents, VGroup,
	Child, HGroup,
	    MUIA_InputMode, MUIV_InputMode_Immediate,
/*              MUIA_ShowSelState, FALSE, */
	    Child, ImageObject,
                MUIA_ShowSelState, FALSE,
	        MUIA_Image_FontMatch, TRUE,
	        MUIA_Image_Spec, MUII_RadioButton,
	        MUIA_Frame, MUIV_Frame_None,
   	        End,
	    Child, TextObject,
                MUIA_ShowSelState, FALSE,
	        MUIA_Text_Contents, "London",
	        MUIA_Frame, MUIV_Frame_None,
	        MUIA_Text_PreParse, "\33l",
	        End,
	End,
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;
/*  #if 0 */
	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

/*  #endif */
	set(wnd, MUIA_Window_Open, TRUE);

/*  #if 0 */
	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}
/*  #endif */
	set(wnd, MUIA_Window_Open, FALSE);
	MUI_DisposeObject(app);
    }
    
    return 0;
}

