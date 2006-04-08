/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
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

char versionstring[] = "$VER: WBNewDrawer 0.5 (05.04.2006) ©2006 AROS Dev Team";

static STRPTR AllocateNameFromLock(BPTR lock);
static void bt_ok_hook_function(void);
static void Cleanup(STRPTR s);
static BOOL doNewDrawer(void);
static void MakeGUI(void);
static const STRPTR SelectDefaultName(STRPTR basename);

static Object *app, *window, *bt_ok, *bt_cancel, *cm_icons, *str_name;
static struct Hook bt_ok_hook;
static BPTR dirlock = (BPTR)-1;
static BPTR oldlock = (BPTR)-1;
static STRPTR illegal_chars = "/:";


int main(int argc, char **argv)
{
    struct WBStartup *startup;

    if (argc != 0)
    {
	PutStr(_(MSG_WB_ONLY));
	Cleanup(NULL);
    }
    startup = (struct WBStartup *) argv;

    D(bug("[NewDrawer] Args %d\n", startup->sm_NumArgs));

    if (startup->sm_NumArgs != 2)
	Cleanup(_(MSG_NEEDS_MORE_ARGS));

    dirlock = startup->sm_ArgList[1].wa_Lock;
    if (dirlock == NULL)
	Cleanup(_(MSG_INVALID_LOCK));

    oldlock = CurrentDir(dirlock);
    MakeGUI();

    STRPTR fullname = AllocateNameFromLock(dirlock);
    UpdateWorkbenchObject(fullname, WBDRAWER, TAG_DONE);
    if (fullname) FreeVec(fullname);

    Cleanup(NULL);
    return RETURN_OK;
}


static void MakeGUI(void)
{
    const STRPTR defname = SelectDefaultName(_(MSG_BASENAME));
    bt_ok_hook.h_Entry = (APTR)bt_ok_hook_function;
    (IPTR)(app = ApplicationObject,
	MUIA_Application_Title      , __(MSG_TITLE),
	MUIA_Application_Version    , (IPTR) versionstring,
	MUIA_Application_Copyright  , __(MSG_COPYRIGHT),
	MUIA_Application_Author     , (IPTR) "The AROS Development Team",
	MUIA_Application_Description, __(MSG_DESCRIPTION),
	MUIA_Application_Base       , (IPTR) "NEWDRAWER",
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
		Child, (IPTR) (ColGroup(2),
		    Child, (IPTR) Label2(__(MSG_NAME)),
		    Child, (IPTR)(str_name = StringObject,
			MUIA_CycleChain, 1,
			MUIA_String_Contents, (IPTR) defname,
			MUIA_String_MaxLen, MAXFILENAMELENGTH,
			MUIA_String_Reject, (IPTR) illegal_chars,
			MUIA_String_Columns, -1,
			MUIA_Frame, MUIV_Frame_String,
		    End),
		    Child, (IPTR) Label2(__(MSG_ICON)),
		    Child, (IPTR)(HGroup,
			Child, (IPTR) (cm_icons = MUI_MakeObject(MUIO_Checkmark, NULL)),
			Child, (IPTR) HVSpace,
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
    if (defname) FreeVec(defname);

    if (!app)
	Cleanup(_(MSG_FAILED_CREATE_APP));

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_CallHook, (IPTR)&bt_ok_hook);
    set(cm_icons, MUIA_CycleChain, 1);
    set(cm_icons, MUIA_Selected, TRUE);
    set(window, MUIA_Window_Open, TRUE);
    DoMethod(app, MUIM_Application_Execute);
}


static void bt_ok_hook_function(void)
{
    if (doNewDrawer())
    {
	DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    }
}


static BOOL doNewDrawer(void)
{
    BOOL retval = FALSE;
    BOOL icon = XGET(cm_icons, MUIA_Selected);
    STRPTR name = (STRPTR)XGET(str_name, MUIA_String_Contents);
    BPTR test;
    struct DiskObject *dob = NULL;
    D(bug("WBNewDrawer name %s icon %d\n", name, icon));

    if (strpbrk(name, illegal_chars))
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_ILLEGAL_CHARS), name);
	goto end;
    }

    test = Lock(name, ACCESS_READ);
    if (test)
    {
	UnLock(test);
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_ALREADY_EXIST), name);
	goto end;
    }

    dob = GetDiskObject(name);
    // if icon exists it must be of type WBDRAWER
    if (dob && (dob->do_Type != WBDRAWER))
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_WRONG_ICON_TYPE), name);
	goto end;
    }

    // create drawer
    BPTR dstLock;
    if ((dstLock  = CreateDir(name)))
	UnLock(dstLock);
    else
    {
	MUI_Request(app, window, 0, _(MSG_ERROR_TITLE), _(MSG_OK), _(MSG_CANT_CREATE), name);
	goto end;
    }
    
    // create icon
    // if the icon already exists and is of right type then no actions happens.
    if (icon &&  (dob == NULL))
    {
	dob = GetDiskObjectNew(name);
	PutDiskObject(name, dob);
    }
    
    UpdateWorkbenchObject(name, WBDRAWER, TAG_DONE);
    retval = TRUE;
    
end:
    if (dob)
	FreeDiskObject(dob);
    return retval;
}


/*
 * Try to find an unique file name.
 * You have to free the return value with FreeVec.
 * */

static const STRPTR SelectDefaultName(STRPTR basename)
{
    if (basename == NULL)
	basename = "Rename_Me";

    BPTR test;
    LONG number = 0;
    STRPTR buffer = AllocVec(strlen(basename) + 3, MEMF_ANY);
    if (!buffer)
	return buffer;
    
    do
    {
	if (number == 0)
	    strcpy(buffer, basename);   
	else
	    sprintf(buffer,"%s_%ld", basename, number);

	test = Lock(buffer, ACCESS_READ);
	UnLock(test);
	number++;
    }
    while ((number < 10) && (test != NULL));
    return buffer;
}


static STRPTR AllocateNameFromLock(BPTR lock)
{
    ULONG  length = 512;
    STRPTR buffer = NULL;
    BOOL   done   = FALSE;

    while (!done)
    {
	if (buffer != NULL) FreeVec(buffer);

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
	if (buffer != NULL) FreeVec(buffer);
	return NULL;
    }
}


static void Cleanup(STRPTR s)
{
    if (app) MUI_DisposeObject(app);

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

