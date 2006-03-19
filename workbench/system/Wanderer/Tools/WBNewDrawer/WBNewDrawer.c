/*
    Copyright © 2006, The AROS Development Team. All rights reserved.
    $Id$
*/

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char versionstring[] = "$VER: WBNewDrawer 0.1 (19.03.2006)";

static void bt_ok_hook_function(void);
static void Cleanup(STRPTR s);
static void MakeGUI(void);
static const STRPTR SelectDefaultName(STRPTR basename);

static Object *app, *window, *bt_ok, *bt_cancel, *cm_icons, *str_name;
static struct Hook bt_ok_hook;
BPTR dirlock = (BPTR)-1;
BPTR oldlock = (BPTR)-1;

int main(int argc, char **argv)
{
    struct WBStartup *startup;

    if (argc != 0)
	Cleanup("Cannot be started from shell\n");

    startup = (struct WBStartup *) argv;

    D(bug("[NewDrawer] Args %d\n", startup->sm_NumArgs));

    if (startup->sm_NumArgs != 2)
	Cleanup("Need 2 arguments\n");

    dirlock = startup->sm_ArgList[1].wa_Lock;
    if (dirlock == NULL)
	Cleanup("Invalid directory lock\n");

    oldlock = CurrentDir(dirlock);
    MakeGUI();
    Cleanup(NULL);
    return RETURN_OK;
}

static void MakeGUI(void)
{
    const STRPTR defname = SelectDefaultName("Rename_Me");
    bt_ok_hook.h_Entry = (APTR)bt_ok_hook_function;
    app = ApplicationObject,
	MUIA_Application_Title      , "WBNewDrawer",
	MUIA_Application_Version    , versionstring,
	MUIA_Application_Copyright  , "©2006, AROS Development Team",
	MUIA_Application_Author     , "AROS Development Team",
	MUIA_Application_Description, "Creates new drawer with optional icon",
	MUIA_Application_Base       , "NEWDRAWER",
	MUIA_Application_UseCommodities, FALSE,
	MUIA_Application_UseRexx, FALSE,
	SubWindow, window = WindowObject,
	    MUIA_Window_Title, "Create new drawer",
	    MUIA_Window_ID   , MAKE_ID('M','K','D','R'),
	    WindowContents, VGroup,
		MUIA_Frame, MUIV_Frame_Group,
		Child, Label(MUIX_C "Enter the name of the new drawer"),
		Child, ColGroup(2),
		    Child, Label("New drawer name:"),
		    Child, str_name = StringObject,
			MUIA_String_Contents, defname,
			MUIA_String_MaxLen, MAXFILENAMELENGTH,
			MUIA_String_Reject, "\":#?*", // Doesn't work :-(
			MUIA_String_Columns, 25,
			MUIA_Frame, MUIV_Frame_String,
		    End,
		    Child, Label("Icon"),
		    Child, cm_icons = MUI_MakeObject(MUIO_Checkmark, NULL),
		End,
		Child, (IPTR) RectangleObject, 
		    MUIA_Rectangle_HBar, TRUE, 
		    MUIA_FixHeight,      2, 
		End,
		Child, HGroup,
		    Child, bt_ok = SimpleButton("OK"),
		    Child, bt_cancel = SimpleButton("Cancel"),
		End,
	    End,
	End,
    End;

    FreeVec(defname);
    if (!app)
	Cleanup("Failed to create Application.");

    DoMethod(window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
	    app, 2, MUIM_CallHook, (IPTR)&bt_ok_hook);
    set(cm_icons, MUIA_Selected, TRUE);
    set(window, MUIA_Window_Open, TRUE);
    DoMethod(app, MUIM_Application_Execute);
}

static void bt_ok_hook_function(void)
{
    BOOL icon = XGET(cm_icons, MUIA_Selected);
    STRPTR name = (STRPTR)XGET(str_name, MUIA_String_Contents);
    BPTR test = 0;
    struct DiskObject *dob;
    D(bug("WBNewDrawer name %s icon %d\n", name, icon));
    
    test = Lock(name, ACCESS_READ);
    if (test)
    {
	UnLock(test);
	MUI_Request(app, window, 0, "WBNewDrawer Error", "OK", "'%s' already exists", name);
	return;
    }
    // create drawer
    BPTR dstLock;
    if ((dstLock  = CreateDir(name)))
	UnLock(dstLock);
    else
	MUI_Request(app, window, 0, "WBNewDrawer Error", "OK", "Can't create drawer '%s'", name);
    

    // create icon
    if (icon)
    {
	dob = GetDiskObject(name);
	if (dob)
	{
	    FreeDiskObject(dob);
	    MUI_Request(app, window, 0, "WBNewDrawer Error", "OK", "'%s'.info already exists", name);
	}
	else
	{
	    dob = GetDiskObjectNew(name);
	    PutDiskObject(name, dob);
	    FreeDiskObject(dob);
	}
    }

    UpdateWorkbenchObject(name, WBDRAWER, NULL);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
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
    do
    {
	number++;
	sprintf(buffer,"%s_%ld", basename, number);
	test = Lock(buffer, ACCESS_READ);
	UnLock(test);
    }
    while ((number < 9) && (test != NULL));
    return buffer;
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
	    es.es_Title = "WBNewDrawer Error";
	    es.es_TextFormat = s;
	    es.es_GadgetFormat = "OK";
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

