/*
    Copyright (C) 2003-2011, The AROS Development Team.
    $Id$
*/

#include <proto/acpica.h>
#include <proto/alib.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <proto/intuition.h>

#include <exec/memory.h>
#include <libraries/asl.h>
#include <libraries/mui.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <aros/debug.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "locale.h"
#include "parsers.h"

#define APPNAME "ACPITool"
#define VERSION "ACPITool 1.0"

const char version[] = "$VER: " VERSION " (" ADATE ")\n";

struct Library *ACPICABase;

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

    CloseLibrary(ACPICABase);
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
Object *ModeCycle;
Object *SaveButton;
Object *menu_quit, *menu_dump_parsed, *menu_dump_raw;

AROS_UFH3(static void, display_function,
    AROS_UFHA(struct Hook *, h,  A0),
    AROS_UFHA(const char **, strings, A2),
    AROS_UFHA(const ACPI_TABLE_HEADER *, table, A1))
{
    AROS_USERFUNC_INIT

    const struct Parser *t = FindParser(table->Signature);

    if (t)
    {
	strings[0] = t->name;
	return;
    }

    snprintf(buf, sizeof(buf), "%s (%4.4s)", _(MSG_UNKNOWN), table->Signature);
    strings[0] = buf;

    AROS_USERFUNC_EXIT
}

static void display_callback(const char *str)
{
    DoMethod(InfoList, MUIM_List_InsertSingle, str, MUIV_List_Insert_Bottom);
}

AROS_UFH3(static void, select_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    IPTR mode, active;
    const ACPI_TABLE_HEADER *table;
    const struct Parser *t;    

    GetAttr(MUIA_Cycle_Active, ModeCycle, &mode);

    SetAttrs(InfoList, MUIA_List_Quiet, TRUE, TAG_DONE);
    DoMethod(InfoList, MUIM_List_Clear);

    GetAttr(MUIA_List_Active, object, &active);
    if (active != MUIV_List_Active_Off)
    {
    	void (*parser)() = unknown_parser;

	DoMethod(object, MUIM_List_GetEntry, active, &table);

	if (mode == 0)
	{
	    t = FindParser(table->Signature);
	    if (t)
	    	parser = t->parser;
	}

	parser(table, display_callback);
    }

    SetAttrs(InfoList, MUIA_List_Quiet, FALSE, TAG_DONE);

    AROS_USERFUNC_EXIT
}

static BPTR RequestFile(void)
{
    BPTR file = BNULL;
    struct FileRequester *req;

    req = MUI_AllocAslRequestTags(ASL_FileRequest, ASLFR_DoSaveMode, TRUE, TAG_DONE);
    if (!req)
    	return BNULL;

    if (MUI_AslRequest(req, NULL))
    {
	ULONG len = strlen(req->fr_Drawer) + strlen(req->fr_File) + 2;
	STRPTR pathname = AllocMem(len, MEMF_ANY);

	if (pathname)
	{
	    strcpy(pathname, req->fr_Drawer);
	    AddPart(pathname, req->fr_File, len);

	    file = Open(pathname, MODE_NEWFILE);
	    FreeMem(pathname, len);
	}
    }
    MUI_FreeAslRequest(req);

    return file;
}

AROS_UFH3(static void, save_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(APTR, msg, A1))
{
    AROS_USERFUNC_INIT

    BPTR file = RequestFile();

    if (file)
    {
	IPTR i, n;

    	GetAttr(MUIA_List_Entries, object, &n);

	for (i = 0; i < n; i++)
    	{
    	    APTR text;

    	    DoMethod(object, MUIM_List_GetEntry, i, &text);
    	    
    	    FPuts(file, text);
    	    FPutC(file, '\n');
    	}
    	Close(file);
    }

    AROS_USERFUNC_EXIT
}    	

BPTR DumpFile;

static void dump_callback(const char *str)
{
    FPuts(DumpFile, str);
    FPutC(DumpFile, '\n');
}

AROS_UFH3(static IPTR, dumpFunc,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(const ACPI_TABLE_HEADER *, table, A2),
	  AROS_UFHA(BOOL, parsed, A1))
{
    AROS_USERFUNC_INIT

    void (*parser)() = unknown_parser;

    if (parsed)
    {
	const struct Parser *t = FindParser(table->Signature);

	if (t)
	    parser = t->parser;
    }

    parser(table, dump_callback);
    FPutC(DumpFile, '\n');
    return TRUE;

    AROS_USERFUNC_EXIT
}

static const struct Hook dumpHook =
{
    .h_Entry = (APTR)dumpFunc
};

AROS_UFH3(static void, dump_function,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, object, A2),
    AROS_UFHA(IPTR *, args, A1))
{
    AROS_USERFUNC_INIT

    /*
     * MUIM_CallHook builds an array from its arguments, and gives us a pointer to this array.
     * We need to explicitly convert to BOOL, otherwise the value can be padded with garbage,
     * which will be contained in IPTR's upper unused bytes.
     */
    BOOL parsed = args[0];

    DumpFile = RequestFile();
    if (DumpFile)
    {
        int i;
        for (i = 0; ParserTable[i].name; i++)
            AcpiScanTables(ParserTable[i].signature, &dumpHook, (APTR)(IPTR)parsed);

	Close(DumpFile);
    }

    AROS_USERFUNC_EXIT
}

static const struct Hook display_hook =
{
    .h_Entry = (APTR)display_function
};

static const struct Hook select_hook =
{
    .h_Entry = (APTR)select_function
};

static const struct Hook save_hook =
{
    .h_Entry = (APTR)save_function
};

static const struct Hook dump_hook =
{
    .h_Entry = (APTR)dump_function
};

static const char *showModes[] =
{
    "Parsed data",
    "Raw dump",
    NULL
};

static BOOL GUIinit()
{
    BOOL retval = FALSE;

    app = ApplicationObject,
	    MUIA_Application_Title,	    (IPTR)APPNAME,
	    MUIA_Application_Version,	    (IPTR)VERSION,
	    MUIA_Application_Copyright,	    (IPTR)"(C) 2004-2011, The AROS Development Team",
	    MUIA_Application_Author,	    (IPTR)"Pavel Fedin",
	    MUIA_Application_Base,	    (IPTR)APPNAME,
	    MUIA_Application_Description,   __(MSG_DESCRIPTION),

            MUIA_Application_Menustrip, MenuitemObject,
            	MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_PROJECT),
                    MUIA_Family_Child,
                    	menu_quit = MenuitemObject, MUIA_Menuitem_Title, __(MSG_MENU_QUIT),
                    End,
            	End,
            	MUIA_Family_Child, MenuitemObject,
                    MUIA_Menuitem_Title, __(MSG_MENU_DUMP),
                    MUIA_Family_Child,
                    	menu_dump_parsed = MenuitemObject,
		    	MUIA_Menuitem_Title, __(MSG_MENU_DUMP_PARSED),
                    End,
                    MUIA_Family_Child,
                    	menu_dump_raw = MenuitemObject,
		    	MUIA_Menuitem_Title, __(MSG_MENU_DUMP_RAW),
                    End,
            	End,
            End,

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
			MUIA_CycleChain, TRUE,
		    End, // ListView

		    Child, VGroup,
		    	Child, ListviewObject,
			    MUIA_Listview_List, InfoList = ListObject,
			    	ReadListFrame,
				MUIA_Font, MUIV_Font_Fixed,
			    	MUIA_List_AdjustWidth, TRUE,
			    	MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
			    	MUIA_List_DestructHook, MUIV_List_DestructHook_String,
			    End, // List
			    MUIA_CycleChain, TRUE,
		    	End, // ListView
		    	Child, HGroup,
		    	    Child, Label(__(MSG_SHOW_MODE)),
		    	    Child, ModeCycle = CycleObject,
		    	    	MUIA_Cycle_Entries, showModes,
		    	    	MUIA_CycleChain, TRUE,
		    	    End,
		    	    Child, SaveButton = SimpleButton(__(MSG_SAVE_DATA)),
		    	End,
		    End,
		End, // WindowContents
	    End, // MainWindow
	End; // ApplicationObject

    if (app)
    {
	/* Quit application if the windowclosegadget or the esc key is pressed. */
	DoMethod(MainWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
		 app, 2, 
		 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	DoMethod(menu_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
		 app, 2,
		 MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	DoMethod(TablesList, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		 TablesList, 2,
		 MUIM_CallHook, &select_hook);

	DoMethod(ModeCycle, MUIM_Notify, MUIA_Cycle_Active, MUIV_EveryTime,
		 TablesList, 2,
		 MUIM_CallHook, &select_hook);

	DoMethod(SaveButton, MUIM_Notify, MUIA_Pressed, FALSE,
		 InfoList, 2,
		 MUIM_CallHook, &save_hook);

	DoMethod(menu_dump_parsed, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
		 app, 3,
		 MUIM_CallHook, &dump_hook, TRUE);

	DoMethod(menu_dump_raw, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime,
		 app, 3,
		 MUIM_CallHook, &dump_hook, FALSE);

	retval = TRUE;
    }

    return retval;
}

AROS_UFH3(static IPTR, tableFunc,
	  AROS_UFHA(struct Hook *, table_hook, A0),
	  AROS_UFHA(const ACPI_TABLE_HEADER *, table, A2),
	  AROS_UFHA(void *, unused, A1))
{
    AROS_USERFUNC_INIT

    DoMethod(TablesList, MUIM_List_InsertSingle, table, MUIV_List_Insert_Bottom);
    return TRUE;

    AROS_USERFUNC_EXIT
}

static const struct Hook tableHook =
{
    .h_Entry = (APTR)tableFunc
};

int __nocommandline = 1;

int main(void)
{
    int i;

    if (!Locale_Initialize())
	cleanup(_(MSG_ERROR_LOCALE));

    ACPICABase = OpenLibrary("acpica.library",0);
    if (!ACPICABase)
	cleanup(_(MSG_ERROR_NO_ACPI));

    if (GUIinit())
    {
	/* Populate tables list */
	for (i = 0; ParserTable[i].name; i++)
	    AcpiScanTables(ParserTable[i].signature, &tableHook, NULL);

	set(MainWindow, MUIA_Window_Open, TRUE);

	if (xget(MainWindow, MUIA_Window_Open))
	{
	    ULONG retval = 0;
	    ULONG signals = 0;

	    while (retval != MUIV_Application_ReturnID_Quit)
	    {
		retval = DoMethod(app, MUIM_Application_NewInput, &signals);

		if (signals)
		    signals = Wait(signals | SIGBREAKF_CTRL_C);

		if (signals & SIGBREAKF_CTRL_C)
		    retval = MUIV_Application_ReturnID_Quit;
	    }
	    set(MainWindow, MUIA_Window_Open, FALSE);
	}

	DisposeObject(app);
    }

    cleanup(NULL);
    return 0;
}
