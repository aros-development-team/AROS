/*
    Copyright � 2002, The AROS Development Team.
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
    Object *wnd, *str, *dirlist, *page;
    
    MUIMasterBase = (struct Library*)OpenLibrary("muimaster.library",0);

    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "dirlist",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
	    	MUIA_Background, MUII_GroupBack,
		Child, ListviewObject,
		    MUIA_Listview_List, dirlist = DirlistObject,
    	    	    	InputListFrame,
		    	End,
		    End,
		Child, HGroup,
		    Child, str = StringObject,
			StringFrame,
			MUIA_String_Contents, (IPTR)"SYS:",
			End,
		    Child, page = PageGroup,
		    	MUIA_Weight, 0,
			MUIA_FixWidthTxt, (IPTR)"AA",
		    	Child, ColorfieldObject,
			    MUIA_Colorfield_Red, 0xFFFFFFFF,
			    MUIA_Colorfield_Green, 0,
			    MUIA_Colorfield_Blue, 0,
			    End,
		    	Child, ColorfieldObject,
			    MUIA_Colorfield_Red, 0xFFFFFFFF,
			    MUIA_Colorfield_Green, 0xFFFFFFFF,
			    MUIA_Colorfield_Blue, 0,
			    End,
		    	Child, ColorfieldObject,
			    MUIA_Colorfield_Red, 0,
			    MUIA_Colorfield_Green, 0x66666666,
			    MUIA_Colorfield_Blue, 0,
			    End,
		    	End,
		    End,
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

    	DoMethod(str, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
	    	 (IPTR)dirlist, 3, MUIM_Set, MUIA_Dirlist_Directory, MUIV_TriggerValue);
		 
    	DoMethod(dirlist, MUIM_Notify, MUIA_Dirlist_Status, MUIV_EveryTime,
	    	 (IPTR)page, 3, MUIM_Set, MUIA_Group_ActivePage, MUIV_TriggerValue);
		 
    	set(dirlist, MUIA_Dirlist_Directory, "SYS:");
	
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

