/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>

#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "windowp.h"
#include "zunestuff.h"

struct Library *MUIMasterBase;

void load_prefs(char *filename);
void save_prefs(char *filename);


#ifndef _AROS

/* On AmigaOS we build a fake library base, because it's not compiled as sharedlibrary yet */
#include "muimaster_intern.h"

int open_muimaster(void)
{
    static struct MUIMasterBase_intern MUIMasterBase_instance;
    MUIMasterBase = (struct Library*)&MUIMasterBase_instance;

    MUIMasterBase_instance.sysbase = *((struct ExecBase **)4);
    MUIMasterBase_instance.dosbase = (void*)OpenLibrary("dos.library",37);
    MUIMasterBase_instance.utilitybase = (void*)OpenLibrary("utility.library",37);
    MUIMasterBase_instance.aslbase = OpenLibrary("asl.library",37);
    MUIMasterBase_instance.gfxbase = (void*)OpenLibrary("graphics.library",37);
    MUIMasterBase_instance.layersbase = OpenLibrary("layers.library",37);
    MUIMasterBase_instance.intuibase = (void*)OpenLibrary("intuition.library",37);
    MUIMasterBase_instance.cxbase = OpenLibrary("commodities.library",37);
    MUIMasterBase_instance.keymapbase = OpenLibrary("keymap.library",37);
    MUIMasterBase_instance.gadtoolsbase = OpenLibrary("gadtools.library",37);
    MUIMasterBase_instance.iffparsebase = OpenLibrary("iffparse.library",37);
    MUIMasterBase_instance.diskfontbase = OpenLibrary("diskfont.library",37);
    __zune_prefs_init(&__zprefs);
    InitSemaphore(&MUIMB(MUIMasterBase)->ZuneSemaphore);
    return 1;
}

void close_muimaster(void)
{
}

#else

int open_muimaster(void)
{
    if ((MUIMasterBase = OpenLibrary("muimaster.library", 0))) return 1;
    return 0;
}

void close_muimaster(void)
{
    if (MUIMasterBase) CloseLibrary(MUIMasterBase);
}

#endif


/* The following two functions should be a method of config data */
static void LoadPrefs(STRPTR filename, Object *obj)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
	if ((iff->iff_Stream = Open(filename,MODE_OLDFILE)))
	{
	    InitIFFasDOS(iff);

	    if (!OpenIFF(iff, IFFF_READ))
	    {
		StopChunk( iff, 'PREF', 'MUIC');

		while (!ParseIFF(iff, IFFPARSE_SCAN))
		{
		    struct ContextNode *cn;
		    if (!(cn = CurrentChunk(iff))) continue;
		    if (cn->cn_ID == 'MUIC') DoMethod(obj,MUIM_Dataspace_ReadIFF,iff);
		}

		CloseIFF(iff);
	    }
	    Close(iff->iff_Stream);
	}
	FreeIFF(iff);
    }
}

static int SavePrefsHeader(struct IFFHandle *iff)
{
    if (!PushChunk( iff, 0, 'PRHD', IFFSIZE_UNKNOWN))
    {
	struct PrefHeader ph;
	ph.ph_Version = 0;
	ph.ph_Type = 0;
	ph.ph_Flags = 0;

	if (WriteChunkBytes(iff, &ph, sizeof(struct PrefHeader)))
	    if (!PopChunk(iff)) return 1;
	PopChunk(iff);
    }
    return 0;
}

static void SavePrefs(STRPTR filename, Object *obj)
{
    struct IFFHandle *iff;
    if ((iff = AllocIFF()))
    {
	if ((iff->iff_Stream = Open(filename,MODE_NEWFILE)))
	{
	    InitIFFasDOS(iff);

	    if (!OpenIFF(iff, IFFF_WRITE))
	    {
		if (!PushChunk(iff, 'PREF', ID_FORM, IFFSIZE_UNKNOWN))
		{
		    if (SavePrefsHeader(iff))
		    {
		    	DoMethod(obj,MUIM_Dataspace_WriteIFF, iff, 0, 'MUIC');
		    }
		    PopChunk(iff);
		}

		CloseIFF(iff);
	    }
	    Close(iff->iff_Stream);
	}
	FreeIFF(iff);
    }
}


/****************************************************************
 Open needed libraries
*****************************************************************/
int open_libs(void)
{
    if (open_muimaster())
    {
	return 1;
    }
    
    return 0;
}

/****************************************************************
 Close opened libraries
*****************************************************************/
void close_libs(void)
{
    close_muimaster();
}

struct Hook hook_standard;

static Object *app;
static Object *open_menuitem;
static Object *append_menuitem;
static Object *saveas_menuitem;
static Object *aboutzune_menuitem;
static Object *quit_menuitem;

static Object *main_wnd;
static Object *main_page_list;
static Object *main_page_group; /* contains the selelected group */
static Object *main_page_group_displayed; /* The current displayed group */
static Object *main_page_space; /* a space object */


struct page_entry
{
    char *name;
    struct MUI_CustomClass *cl; /* The class pointer,  maybe NULL */
    Object *group;  /* The group which should be is displayed, maybe NULL */
    const struct __MUIBuiltinClass *desc;
};

struct page_entry main_page_entries[] =
{
    {"Info",NULL,NULL,NULL},
    {"System",NULL,NULL,NULL},
    {"Windows",NULL,NULL,&_MUIP_Windows_desc},
};

#define MAIN_PAGE_ENTRIES_LEN sizeof(main_page_entries)/sizeof(main_page_entries[0])

struct MUI_CustomClass *create_class(const struct __MUIBuiltinClass *desc)
{
    return MUI_CreateCustomClass(NULL,MUIC_Settingsgroup,NULL,desc->datasize,desc->dispatcher);
}

/****************************************************************
 Our standard hook function, for easy call backs
*****************************************************************/
#ifndef __AROS__
__saveds static __asm void hook_func_standard(register __a0 struct Hook *h, register __a1 ULONG * funcptr)
#else
AROS_UFH3( void, hook_func_standard,
    AROS_UFHA( struct Hook *, h,       A0 ),
    AROS_UFHA( Object *, obj, A2),
    AROS_UFHA( ULONG *,       funcptr, A1 ))
#endif
{
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);
    if (func) func(funcptr + 1);
}

/****************************************************************
 The display function for the page listview
*****************************************************************/
#ifndef __AROS__
__saveds __asm void main_page_list_display(register __a0 struct Hook *h, register __a2 char **strings, register __a1 struct page_entry *entry)
#else
AROS_UFH3( void, main_page_list_display,
    AROS_UFHA( struct Hook *,       h,       A0 ),
    AROS_UFHA( char **,             strings, A2 ),
    AROS_UFHA( struct page_entry *, entry,   A1 )) 
#endif
{
    if (entry)
    {
        strings[0] = entry->name;
    }
}


/****************************************************************
 A new entry has been selected
*****************************************************************/
void main_page_active(void)
{
    int new_active = xget(main_page_list,MUIA_List_Active);
    Object *new_group;

    if (new_active == -1) new_group = main_page_space;
    else
    {
	new_group = main_page_entries[new_active].group;
	if (!new_group) new_group = main_page_space;
    }

    if (new_group == main_page_group_displayed) return;

    DoMethod(main_page_group,MUIM_Group_InitChange);
    DoMethod(main_page_group,OM_REMMEMBER,main_page_group_displayed);
    DoMethod(main_page_group,OM_ADDMEMBER,new_group);
    DoMethod(main_page_group,MUIM_Group_ExitChange);
    main_page_group_displayed = new_group;
}

/****************************************************************
 Save pressed
*****************************************************************/
void main_save_pressed(void)
{
    save_prefs("env:zune/global.prefs");
    save_prefs("envarc:zune/global.prefs");
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/****************************************************************
 Use pressed
*****************************************************************/
void main_use_pressed(void)
{
    save_prefs("env:zune/global.prefs");
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/****************************************************************
 Allocalte resources for gui
*****************************************************************/
int init_gui(void)
{
    Object *save_button;
    Object *use_button;
    Object *test_button;
    Object *cancel_button;

    static struct Hook page_display_hook;

    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    page_display_hook.h_Entry = (HOOKFUNC)main_page_list_display;

    app = ApplicationObject,
	MUIA_Application_Menustrip, MenuitemObject,
	    MUIA_Family_Child, MenuitemObject,
	    	MUIA_Menuitem_Title, "Project",
	    	MUIA_Family_Child, open_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Open...", MUIA_Menuitem_Shortcut, "O", End,
	    	MUIA_Family_Child, append_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Append...", End,
	    	MUIA_Family_Child, saveas_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Save As...", MUIA_Menuitem_Shortcut, "A", End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, ~0, End,
	    	MUIA_Family_Child, aboutzune_menuitem = MenuitemObject, MUIA_Menuitem_Title, "About Zune...", MUIA_Menuitem_Shortcut, "?", End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, ~0, End,
	    	MUIA_Family_Child, quit_menuitem = MenuitemObject, MUIA_Menuitem_Title, "Quit", MUIA_Menuitem_Shortcut, "Q", End,
	    	End,
	    End,
    	SubWindow, main_wnd = WindowObject,
    	    MUIA_Window_Title, "Zune - Preferences",
	    MUIA_Window_Activate, TRUE,

	    WindowContents, VGroup,
    	    	Child, HGroup,
		    Child, ListviewObject,
			MUIA_Listview_List, main_page_list = ListObject,
			    InputListFrame,
			    MUIA_List_DisplayHook, &page_display_hook,
			    End,
			End,
		    Child, main_page_group = VGroup,
			Child, main_page_group_displayed = main_page_space = HVSpace,
		        End,
		    End,
    	    	Child, HGroup,
		    Child, save_button = MakeButton("Save"),
		    Child, use_button = MakeButton("Use"),
    	    	    Child, test_button = MakeButton("Test"),
    	    	    Child, cancel_button = MakeButton("Cancel"),
    	    	    End,
		End,
    	    End,
    	End;

    if (app)
    {
    	int i;

	DoMethod(main_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
	DoMethod(save_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, main_save_pressed);
	DoMethod(use_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, main_use_pressed);
//	DoMethod(test_button, MUIM_Notify, MUIA_Pressed, FALSE, app, 3, MUIM_CallHook, &hook_standard, main_test_pressed);
	DoMethod(quit_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

	for (i=0;i<MAIN_PAGE_ENTRIES_LEN;i++)
	{
	    struct page_entry *p = &main_page_entries[i];
	    if (p->desc)
	    {
		if ((p->cl = create_class(p->desc)))
		{
		    p->group = NewObject(p->cl->mcc_Class,NULL,TAG_DONE);
		}
	    }
	    DoMethod(main_page_list,MUIM_List_InsertSingle,p,MUIV_List_Insert_Bottom);
	}

	DoMethod(main_page_list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, app, 3, MUIM_CallHook, &hook_standard, main_page_active);

	/* Activate first entry */
	set(main_page_list,MUIA_List_Active,0);

	return 1;
    }
    return 0;
}

/****************************************************************
 Deallocates all gui resources
*****************************************************************/
void deinit_gui(void)
{
    if (app) MUI_DisposeObject(app);
}

/****************************************************************
 Load the given prefs
*****************************************************************/
void load_prefs(char *filename)
{
    Object *configdata = MUI_NewObjectA(MUIC_Dataspace,NULL);
    if (configdata)
    {
	int i;

	LoadPrefs(filename,configdata);

        /* Call MUIM_Settingsgroup_ConfigToGadgets for every group */
	for (i=0;i<MAIN_PAGE_ENTRIES_LEN;i++)
	{
	    struct page_entry *p = &main_page_entries[i];
	    if (p->group)
		DoMethod(p->group,MUIM_Settingsgroup_ConfigToGadgets,configdata);
	}

    	MUI_DisposeObject(configdata);
    }
}

/****************************************************************
 Saves the done prefs
*****************************************************************/
void save_prefs(char *filename)
{
    Object *configdata = MUI_NewObjectA(MUIC_Dataspace,NULL);
    if (configdata)
    {
	int i;

        /* Call MUIM_Settingsgroup_GadgetsToConfig for every group */
	for (i=0;i<MAIN_PAGE_ENTRIES_LEN;i++)
	{
	    struct page_entry *p = &main_page_entries[i];
	    if (p->group)
		DoMethod(p->group,MUIM_Settingsgroup_GadgetsToConfig,configdata);
	}

	SavePrefs(filename,configdata);

    	MUI_DisposeObject(configdata);
    }
}


/****************************************************************
 The message loop
*****************************************************************/
void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
    {
	if (sigs)
	{
	    sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
	    if (sigs & SIGBREAKF_CTRL_C) break;
	    if (sigs & SIGBREAKF_CTRL_D) break;
	}
    }
}

/****************************************************************
 The main entry point
*****************************************************************/
void main(void)
{
    if (open_libs())
    {
    	if (init_gui())
    	{
    	    load_prefs("env:zune/global.prefs");
    	    set(main_wnd, MUIA_Window_Open, TRUE);
    	    if (xget(main_wnd,MUIA_Window_Open))
	    {
		loop();
	    }

	    deinit_gui();
    	}
	close_libs();
    }
}
