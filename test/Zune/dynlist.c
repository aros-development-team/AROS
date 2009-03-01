/*
    Copyright © 2009, The AROS Development Team. All rights reserved.
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

#include <utility/hooks.h>
#include <libraries/mui.h>

Object *app;
Object *wnd, *str1, *str2, *str3, *bt1, *bt2, *bt3, *list;

static IPTR DynlistDisplayFunc(struct Hook *hook, char **columns, char **entry)
{
    if(entry == NULL)
    {
	columns[0] = "Column 1";
	columns[1] = "Column 2";
	columns[2] = "Column 3";
    }
    else
    {
	columns[0] = entry[0];
	columns[1] = entry[1];
	columns[2] = entry[2];
    }
    return TRUE;
}

static IPTR AddFunc(struct Hook *hook, Object *caller, void *data)
{
    STRPTR s1 = (STRPTR) XGET(str1, MUIA_String_Contents);
    STRPTR s2 = (STRPTR) XGET(str2, MUIA_String_Contents);
    STRPTR s3 = (STRPTR) XGET(str3, MUIA_String_Contents);
    STRPTR *item = (STRPTR*) malloc(sizeof(STRPTR) * 4);
    item[0] = strdup(s1);
    item[1] = strdup(s2);
    item[2] = strdup(s3);
    item[3] = NULL;
    DoMethod(list, MUIM_List_InsertSingle, item, MUIV_List_Insert_Top);
    
    return TRUE;
}

static IPTR MutateFunc(struct Hook *hook, Object *caller, void *data)
{
    LONG active;
    char **entry;
    get(list, MUIA_List_Active, &active);
    if (active == MUIV_List_Active_Off)
	return FALSE;

    DoMethod(list, MUIM_List_GetEntry, active, &entry);
    entry[0] = "A very long entry";
    entry[1] = "This is even longer";
    entry[2] = "I'm the longest entry in the application";
    DoMethod(list, MUIM_List_Redraw, MUIV_List_Redraw_Active);
    
    return TRUE;
}

int main(void)
{    
    struct Hook addHook, displayHook, mutateHook;
    addHook.h_Entry = HookEntry;
    addHook.h_SubEntry = (HOOKFUNC) AddFunc;
    displayHook.h_Entry = HookEntry;
    displayHook.h_SubEntry = (HOOKFUNC) DynlistDisplayFunc;
    mutateHook.h_Entry = HookEntry;
    mutateHook.h_SubEntry = (HOOKFUNC) MutateFunc;
    
    app = ApplicationObject,
   	SubWindow, wnd = WindowObject,
    	    MUIA_Window_Title, "Dynamic list",
	    MUIA_Window_Activate, TRUE,

    	    WindowContents, VGroup,
    	    	Child, list = ListObject,
		    MUIA_List_DisplayHook, &displayHook,
		    MUIA_List_Format, "BAR,BAR,",
		    MUIA_List_Title, TRUE,
		    End,
		Child, HGroup,
		    Child, str1 = StringObject, End,
		    Child, str2 = StringObject, End,
		    Child, str3 = StringObject, End,
		    End,
		Child, HGroup,
		    Child, bt1 = SimpleButton("Add"),
		    Child, bt2 = SimpleButton("Remove"),
		    Child, bt3 = SimpleButton("Grow"),
		    End,
		End,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
	    app, (IPTR) 2, 
	    MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	DoMethod(bt1, MUIM_Notify, MUIA_Pressed, FALSE,
		(IPTR) bt1, 3,
		MUIM_CallHook, &addHook, list);

	DoMethod(bt2, MUIM_Notify, MUIA_Pressed, FALSE,
		(IPTR) list, 2,
		MUIM_List_Remove, MUIV_List_Remove_First);

        DoMethod(bt3, MUIM_Notify, MUIA_Pressed, FALSE,
                (IPTR) list, 3,
                MUIM_CallHook, &mutateHook, NULL);

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
    
    return 0;
}

