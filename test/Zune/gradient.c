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

    app = ApplicationObject,
        SubWindow, wnd = WindowObject,
            MUIA_Window_Activate, TRUE,
            MUIA_Window_Title,    (IPTR)"Gradient-o-matic",

            WindowContents, VGroup,
                MUIA_Background, (IPTR)"7:h,8d8d8d8d,b5b5b5b5,babababa-80808080,96969696,99999999",
                Child, SimpleButton("Hola"),
                Child, StringObject,
                    StringFrame,
                End,
                Child, ImageObject,
                    ButtonFrame,
                    MUIA_Image_Spec, (IPTR)"7:v,6d6d6d6d,b5b5b5b5,babababa-60606060,96969696,99999999",
                    MUIA_Image_FreeHoriz, TRUE,
                    MUIA_Image_FreeVert,  TRUE,
                End,
	        Child, TextObject,
 	            ButtonFrame,
	            MUIA_Background, (IPTR)"7:v,bbbbbbbb,bbbbbbbb,bbbbbbbb-77777777,77777777,77777777",
	            MUIA_Text_Contents, "\033cWoho I see colors on the wall ...",
	        End,
            End,
        End,
    End;

    if (app)
    {
	ULONG sigs = 0;
	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	set(wnd, MUIA_Window_Open, TRUE);

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}
	set(wnd, MUIA_Window_Open, FALSE);
	MUI_DisposeObject(app);
    }
    
    return 0;
}

