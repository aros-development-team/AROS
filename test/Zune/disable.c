/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
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

#include <libraries/mui.h>


int main(void)
{
    Object *application, *window, *b1, *b2, *b3;
    
    application = ApplicationObject,
   	SubWindow, window = WindowObject,
    	    MUIA_Window_Title,    (IPTR) "Disable",
	    MUIA_Window_Activate,        TRUE,
            
    	    WindowContents, (IPTR) VGroup,
                Child, (IPTR) (b1 = ImageButton("Test", "THEME:Images/Gadgets/Prefs/Test")),
                Child, (IPTR) HGroup,
                    Child, (IPTR) (b2 = SimpleButton("Disable")),
                    Child, (IPTR) (b3 = SimpleButton("Enable")),
                End,
            End,
        End,
    End;

    if (application)
    {
	ULONG sigs = 0;

	DoMethod
        (
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) application, 2, MUIM_Application_ReturnID,
            MUIV_Application_ReturnID_Quit
        );
        
        DoMethod
        (
            b2, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) b1, 3, MUIM_Set, MUIA_Disabled, TRUE
        );
        
        DoMethod
        (
            b3, MUIM_Notify, MUIA_Pressed, FALSE,
            (IPTR) b1, 3, MUIM_Set, MUIA_Disabled, FALSE
        );

	set(window,MUIA_Window_Open,TRUE);

	while
        (
            DoMethod
            (
                application, MUIM_Application_NewInput, (IPTR) &sigs
            ) != MUIV_Application_ReturnID_Quit
        )
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(application);
    }

    return 0;
}

