/*
    Copyright (C) 2003-2011, The AROS Development Team.
    $Id$
*/

#include <exec/memory.h>
#include <libraries/mui.h>
#include <resources/acpi.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/acpi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <aros/debug.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "locale.h"
#include "parsers.h"

#define APPNAME "ACPITool"
#define VERSION "ACPITool 0.1"

#define IDB_SAVE 10001
#define IDB_SAVEALL 10002

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

APTR ACPIBase;

static void ShowError(Object *application, Object *window, CONST_STRPTR message, BOOL useIOError)
{
    TEXT   buffer[128];
    STRPTR newline = "\n",
           period  = ".",
           extra   = buffer;
           
    /* Never use IO error if it is 0 */
    if (IoErr() == 0) useIOError = FALSE;
    
    if (useIOError)
    {
        Fault(IoErr(), NULL, buffer, sizeof(buffer));
        buffer[0] = toupper(buffer[0]);
    }
    else
    {
        newline = "";
        period  = "";
        extra   = "";
    }
            
    MUI_Request
    (
        application, window, 0, _(MSG_TITLE), _(MSG_ERROR_OK), 
        "%s:\n%s%s%s%s", _(MSG_ERROR_HEADER), message, newline, extra, period
    );
}

void cleanup(CONST_STRPTR message)
{
    Locale_Deinitialize();

    if (message != NULL)
    {
	ShowError(NULL, NULL, message, TRUE);
	exit(RETURN_FAIL);
    }
}

Object *MakeLabel(STRPTR str)
{
    return (MUI_MakeObject(MUIO_Label, str, 0));
}

static inline LONG xget(Object * obj, ULONG attr)
{
    IPTR x = 0;

    GetAttr(attr, obj, &x);
    return x;
}

Object *app;
Object *MainWindow;
Object *TablesList;
Object *InfoList;

struct Hook display_hook;
struct Hook select_hook;

AROS_UFH3(void, display_function,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(char **, strings, A2),
    AROS_UFHA(struct ACPI_TABLE_DEF_HEADER *, table, A1))
{
    AROS_USERFUNC_INIT

    const struct Parser *t = FindParser(table->signature);

    if (t)
    {
	strings[0] = t->name;
	return;
    }

    snprintf(buf, sizeof(buf), "%s (%4.4s)", _(MSG_UNKNOWN), (char *)&table->signature);
    strings[0] = buf;

    AROS_USERFUNC_EXIT
}

static void display_callback(const char *str)
{
    DoMethod(InfoList, MUIM_List_InsertSingle, str, MUIV_List_Insert_Bottom);
}

AROS_UFH3(void, select_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR active;
    struct ACPI_TABLE_DEF_HEADER *table;
    const struct Parser *t;
    void (*parser)();

    SetAttrs(InfoList, MUIA_List_Quiet, TRUE, TAG_DONE);
    DoMethod(InfoList, MUIM_List_Clear);

    GetAttr(MUIA_List_Active, object, &active);
    if (active != MUIV_List_Active_Off)
    {
	DoMethod(object, MUIM_List_GetEntry, active, &table);
	t = FindParser(table->signature);
	parser = t ? t->parser : header_parser;
	parser(table, display_callback);
    }

    SetAttrs(InfoList, MUIA_List_Quiet, FALSE, TAG_DONE);

    AROS_USERFUNC_EXIT
}

BOOL GUIinit()
{
    BOOL retval = FALSE;

    app = ApplicationObject,
	    MUIA_Application_Title,	    (IPTR)APPNAME,
	    MUIA_Application_Version,	    (IPTR)VERSION,
	    MUIA_Application_Copyright,	    (IPTR)"(C) 2004-2011, The AROS Development Team",
	    MUIA_Application_Author,	    (IPTR)"Pavel Fedin",
	    MUIA_Application_Base,	    (IPTR)APPNAME,
	    MUIA_Application_Description,   __(MSG_DESCRIPTION),

	    SubWindow, MainWindow = WindowObject,
		MUIA_Window_Title,	__(MSG_WINTITLE),

		WindowContents, HGroup,
		    MUIA_Group_SameWidth, FALSE,

		    Child, ListviewObject,
			MUIA_Listview_List, TablesList = ListObject,
			    InputListFrame,
			    MUIA_List_AdjustWidth, TRUE,
			    MUIA_List_DisplayHook, &display_hook,
			End, // List
		    End, // ListView

		    Child, ListviewObject,
			MUIA_Listview_List, InfoList = ListObject,
			    ReadListFrame,
			    MUIA_List_AdjustWidth, TRUE,
			    MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
			    MUIA_List_DestructHook, MUIV_List_DestructHook_String,
			End, // List
		    End, // ListView
		End, // WindowContents
	    End, // MainWindow
	End; // ApplicationObject

    if (app)
    {
	/* Quit application if the windowclosegadget or the esc key is pressed. */
	DoMethod(MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
		 app, 2, 
		 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	DoMethod(TablesList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		 TablesList, 2,
		 MUIM_CallHook, &select_hook);

	retval = TRUE;
    }

    return retval;
}

int __nocommandline = 1;

int main(void)
{
    display_hook.h_Entry = (APTR)display_function;
    select_hook.h_Entry = (APTR)select_function;

    if (!Locale_Initialize())
	cleanup(_(MSG_ERROR_LOCALE));

    ACPIBase = OpenResource("acpi.resource");
    if (!ACPIBase)
	cleanup(_(MSG_ERROR_NO_ACPI));

    if(GUIinit())
    {
	unsigned int i;

	/* Populate tables list */
	DoMethod(TablesList, MUIM_List_InsertSingle, ACPI->ACPIB_SDT_Addr, MUIV_List_Insert_Bottom);
	for (i = 0; i < ACPI->ACPIB_SDT_Count; i++)
	    DoMethod(TablesList, MUIM_List_InsertSingle, ACPI->ACPIB_SDT_Entry[i], MUIV_List_Insert_Bottom);

	set(MainWindow, MUIA_Window_Open, TRUE);

	if (xget(MainWindow, MUIA_Window_Open))
	{
	    DoMethod(app, MUIM_Application_Execute);
	    set(MainWindow, MUIA_Window_Open, FALSE);
	}

	DisposeObject(app);

    }

    cleanup(NULL);

    return 0;
}
