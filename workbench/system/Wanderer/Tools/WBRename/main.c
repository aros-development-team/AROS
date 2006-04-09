/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>
#include <proto/dos.h>
#include <proto/icon.h>

#include <dos/dos.h>
#include <libraries/mui.h>
#include <workbench/startup.h>

#include "locale.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char versionstring[] = "$VER: WBRename 0.3 (10.04.2006) ©2006 AROS Dev Team";

static STRPTR AllocateNameFromLock(BPTR lock);
static void bt_ok_hook_function(void);
static void Cleanup(STRPTR s);
static BOOL doRename(const STRPTR oldname, const STRPTR newname);
static void MakeGUI(void);

static Object *app, *window, *bt_ok, *bt_cancel, *str_name;
static struct Hook bt_ok_hook;
static BPTR parentlock = (BPTR)-1;
static STRPTR oldname;
static BPTR oldlock = (BPTR)-1;
static STRPTR illegal_chars = "/:";


int main(int argc, char **argv)
{
    struct WBStartup *startup;
    STRPTR fullname;

    if (argc != 0)
    {
	PutStr(_(MSG_WB_ONLY));
	Cleanup(NULL);
    }
    startup = (struct WBStartup *) argv;

    D(bug("[WBRename] Args %d\n", startup->sm_NumArgs));

    if (startup->sm_NumArgs != 2)
	Cleanup(_(MSG_NEEDS_MORE_ARGS));

    parentlock = startup->sm_ArgList[1].wa_Lock;
    oldname    = startup->sm_ArgList[1].wa_Name;
    if ((parentlock == NULL) || (oldname == NULL))
	Cleanup(_(MSG_INVALID_LOCK));

    oldlock = CurrentDir(parentlock);
    
    MakeGUI();
    DoMethod(app, MUIM_Application_Execute);

    fullname = AllocateNameFromLock(parentlock);
    UpdateWorkbenchObject(fullname, WBDRAWER, TAG_DONE);
    FreeVec(fullname);

    Cleanup(NULL);
    return RETURN_OK;
}


static void MakeGUI(void)
{
    bt_ok_hook.h_Entry = (APTR)bt_ok_hook_function;
    (IPTR)(app = ApplicationObject,
	MUIA_Application_Title      , __(MSG_TITLE),
	MUIA_Application_Version    , (IPTR) versionstring,
	MUIA_Application_Copyright  , __(MSG_COPYRIGHT),
	MUIA_Application_Author     , (IPTR) "The AROS Development Team",
	MUIA_Application_Description, __(MSG_DESCRIPTION),
	MUIA_Application_Base       , (IPTR) "WBRENAME",
	MUIA_Application_UseCommodities, FALSE,
	MUIA_Application_UseRexx, FALSE,
	SubWindow, (IPTR)(window = WindowObject,
	    MUIA_Window_Title, __(MSG_WINDOW_TITLE),
	    MUIA_Window_NoMenus, TRUE,
	    MUIA_Window_CloseGadget, FALSE,
	    WindowContents, (IPTR) (VGroup,
		MUIA_Frame, MUIV_Frame_Group,
		Child, (IPTR) (HGroup,
		    Child, (IPTR) HVSpace,
		    Child, (IPTR) Label2(__(MSG_LINE)),
		End),
		Child, (IPTR) (HGroup,
		    Child, (IPTR) Label2(__(MSG_NAME)),
		    Child, (IPTR)(str_name = StringObject,
			MUIA_CycleChain, 1,
			MUIA_String_Contents, (IPTR) oldname,
			MUIA_String_MaxLen, MAXFILENAMELENGTH,
			MUIA_String_Reject, (IPTR) illegal_chars, // Doesn't work :-(
			MUIA_String_Columns, -1,
			MUIA_Frame, MUIV_Frame_String,
		    End),
		End),
		Child, (IPTR) (RectangleObject, 
		    MUIA_Rectangle_HBar, TRUE,
		    MUIA_FixHeight,      2,
		End),
		Child, (IPTR) (HGroup,
		    Child, (IPTR) (bt_ok = ImageButton(__(MSG_OK), "THEME:Images/Gadgets/Prefs/Save")),
		    Child, (IPTR) (bt_cancel = ImageButton(__(MSG_CANCEL),"THEME:Images/Gadgets/Prefs/Cancel")),
		End),
	    End),
	End),
    End);
    if (!app)
	Cleanup(_(MSG_FAILED_CREATE_APP));

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_CallHook, (IPTR)&bt_ok_hook);
    set(window, MUIA_Window_Open, TRUE);
}


static void bt_ok_hook_function(void)
{
    STRPTR newname = (STRPTR)XGET(str_name, MUIA_String_Contents);
    D(bug("WBRename oldname %s newname %s \n", oldname, newname));

    if (doRename(oldname, newname))
    {
	DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    }
}


static BOOL doRename(const STRPTR oldname, const STRPTR newname)
{
    BOOL retval = FALSE;
    if (( oldname == NULL) || (newname == NULL))
	return retval;

    STRPTR oldinfoname = NULL;
    STRPTR newinfoname = NULL;
    BOOL infoexists=FALSE;
    BPTR test;

    oldinfoname = AllocVec(strlen(oldname) + 6, MEMF_ANY);
    if (!oldinfoname)
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_OUTOFMEMORY));
	goto end;
    }	
    strcpy(oldinfoname, oldname);
    strcat(oldinfoname, ".info");

    newinfoname = AllocVec(strlen(newname) + 6, MEMF_ANY);
    if (!newinfoname)
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_OUTOFMEMORY));
	goto end;
    }	
    strcpy(newinfoname, newname);
    strcat(newinfoname, ".info");

    if (strpbrk(newname, illegal_chars))
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_ILLEGAL_CHARS), newname);
	goto end;
    }

    if ((test = Lock(newname, ACCESS_READ)))
    {
	UnLock(test);
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_ALREADY_EXIST), newname);
	goto end;
    }

    if ((test = Lock(oldinfoname, ACCESS_READ)))
    {
	UnLock(test);
	infoexists = TRUE; // we have an .info file
	test = Lock(newinfoname, ACCESS_READ);
	if (test)
	{
	    UnLock(test);
	    MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_ALREADY_EXIST), newinfoname);
	    goto end;
	}
    }

    if (Rename(oldname, newname) == DOSFALSE)
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_FAILED), oldname, GetDosErrorString(IoErr()));
	goto end;
    }

    if (infoexists)
    {
	if ( Rename(oldinfoname, newinfoname) == DOSFALSE)
	{
	    MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_FAILED), oldinfoname, GetDosErrorString(IoErr()));
	    goto end;
	}
    }

    retval = TRUE;

end:
    FreeVec(oldinfoname);
    FreeVec(newinfoname);
    return retval;
}


static STRPTR AllocateNameFromLock(BPTR lock)
{
    ULONG  length = 512;
    STRPTR buffer = NULL;
    BOOL   done   = FALSE;

    while (!done)
    {
	FreeVec(buffer);

	buffer = AllocVec(length, MEMF_ANY);
	if (buffer != NULL)
	{
	    if (NameFromLock(lock, buffer, length))
	    {
		done = TRUE;
		break;
	    }
	    else
	    {
		if (IoErr() == ERROR_LINE_TOO_LONG)
		{
		    length += 512;
		    continue;
		}
		else
		{
		    break;
		}
	    }
	}
	else
	{
	    SetIoErr(ERROR_NO_FREE_STORE);
	    break;
	}
    }

    if (done)
    {
	return buffer;
    }
    else
    {
	FreeVec(buffer);
	return NULL;
    }
}


static void Cleanup(STRPTR s)
{
    MUI_DisposeObject(app);

    if (oldlock != (BPTR)-1)
	CurrentDir(oldlock);

    if (s)
    {
	if (IntuitionBase)
	{
	    struct EasyStruct es;
	    es.es_StructSize = sizeof(struct EasyStruct);
	    es.es_Flags = 0;
	    es.es_Title = _(MSG_ERROR_TITLE);
	    es.es_TextFormat = s;
	    es.es_GadgetFormat = _(MSG_OK);
	    EasyRequest(NULL, &es, NULL, NULL);
	}
	else
	{
	    PutStr(s);
	}
	exit(RETURN_ERROR);
    }
    exit(RETURN_OK);
}

