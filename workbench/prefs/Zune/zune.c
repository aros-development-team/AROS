/*
    Copyright  2002-2012, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <string.h>
#include <stdio.h>

#include <exec/memory.h>

#include <libraries/asl.h>
#include <libraries/mui.h>
#include <prefs/prefhdr.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/muimaster.h>

#include "systemp.h"
#include "buttonsp.h"
#include "groupsp.h"
#include "windowp.h"
#include "cyclesp.h"
#include "slidersp.h"
#include "scrollbarsp.h"
#include "listviewsp.h"
#include "stringsp.h"
#include "specialp.h"
#include "navigationp.h"
#include "framesp.h"
#include "zunestuff.h"

/************************************************************************/

#ifndef MAXFILENAMELENGTH
#define MAXFILENAMELENGTH 255
#endif

/************************************************************************/

#ifdef __AROS__
#define MCC_Query(x) AROS_LVO_CALL1(struct MUI_CustomClass *,          \
		                    AROS_LCA(LONG, (x), D0),           \
				    struct Library *, mcclib, 5, lib)
#else

#ifdef __amigaos4__
#else
struct MUI_CustomClass *MCC_Query(ULONG d0);
#pragma  libcall mcclib MCC_Query 01e 001
#endif
#endif

extern IPTR ClassListview_Dispatcher();
struct MUI_CustomClass *ClassListview_CLASS;

#define ZUNEVERSION "$VER: Zune 0.2 (22.02.2006) AROS Dev Team"

APTR *appaddr;

struct TagItem prefstags[] =
        {
        { ASLFR_DoPatterns,         (IPTR)TRUE       },
        { ASLFR_InitialPattern,     (IPTR)"#?.prefs" },
        { TAG_DONE,                 0                },
        };

/************************************************************************/

void load_prefs(CONST_STRPTR name);
void save_prefs(CONST_STRPTR name, BOOL envarc);
void test_prefs(void);
void main_open_menu(void);
void main_saveas_menu(void);
void restore_prefs(CONST_STRPTR name);

/************************************************************************/

#ifdef __amigaos4__
struct Library *ZuneMasterBase;
struct ZuneMasterIFace *IZuneMaster;
#endif


/************************************************************************/

/****************************************************************
 Open needed libraries
*****************************************************************/
int open_libs(void)
{
#ifdef __amigaos4__
    if (!(ZuneMasterBase = OpenLibrary("zunemaster.library",0)))
    {
    	printf("Unable to open zunemaster.library\n");
    	return 0;
    }

    if (!(IZuneMaster = (struct ZuneMasterIFace*)GetInterface(ZuneMasterBase,"main",1,NULL)))
    {
    	printf("Unable to get main interface of zunemaster.library\n");
    	CloseLibrary(ZuneMasterBase); ZuneMasterBase = NULL;
    	return 0;
    }
#endif
    return 1;
}

/****************************************************************
 Close opened libraries
*****************************************************************/
void close_libs(void)
{
#ifdef __amigaos4__
    DropInterface((struct Interface*)IZuneMaster);
    CloseLibrary(ZuneMasterBase);
#endif
}

struct Hook hook_standard;

static Object *app;
static Object *open_menuitem;
static Object *append_menuitem;
static Object *saveas_menuitem;
static Object *aboutzune_menuitem;
static Object *quit_menuitem;
static Object *LastSavedConfigdata = NULL;
static STRPTR appname = NULL;

static Object *main_wnd;
static Object *main_page_list;
static Object *main_page_group; /* contains the selelected group */
static Object *main_page_group_displayed; /* The current displayed group */
static Object *main_page_space; /* a space object */

void close_classes(void)
{
    MUI_DeleteCustomClass(ClassListview_CLASS);
}

int open_classes(void)
{
     if (1)
     {
         ClassListview_CLASS = MUI_CreateCustomClass(NULL, MUIC_Listview, NULL, sizeof(struct ClassListview_DATA), ClassListview_Dispatcher);
	 return 1;
     }
     else
     {
	 close_classes();
	 return 0;
     }
}


struct page_entry
{
    char *name;
    struct MUI_CustomClass *cl; /* The class pointer,  maybe NULL */
    Object *group;  /* The group which should be is displayed, maybe NULL */
    const struct __MUIBuiltinClass *desc;
    struct Library *mcp_library;
    UBYTE mcp_namebuffer[MAXFILENAMELENGTH + 1];
};

#define MAX_PAGE_ENTRIES 100

/* the name field is set in init_gui() */
struct page_entry main_page_entries[MAX_PAGE_ENTRIES + 1] =
{
/*      {"Info",NULL,NULL,NULL}, */
    { "",   NULL, NULL, &_MUIP_System_desc     },
    { "",   NULL, NULL, &_MUIP_Windows_desc    },
    { "",   NULL, NULL, &_MUIP_Groups_desc     },
    { "",   NULL, NULL, &_MUIP_Buttons_desc    },
    { "",   NULL, NULL, &_MUIP_Cycles_desc     },
    { "",   NULL, NULL, &_MUIP_Sliders_desc    },
    { "",   NULL, NULL, &_MUIP_Scrollbars_desc },
    { "",   NULL, NULL, &_MUIP_Listviews_desc  },
    { "",   NULL, NULL, &_MUIP_Strings_desc    },
    { "",   NULL, NULL, &_MUIP_Navigation_desc },
    { "",   NULL, NULL, &_MUIP_Special_desc    },
    { "",   NULL, NULL, &_MUIP_Frames_desc     },
    { NULL, NULL, NULL, NULL                   },
};

struct MUI_CustomClass *create_class(const struct __MUIBuiltinClass *desc)
{
    return MUI_CreateCustomClass(NULL,MUIC_Settingsgroup,NULL,desc->datasize,desc->dispatcher);
}

/****************************************************************
 Our standard hook function, for easy call backs
*****************************************************************/
static void hook_func_standard(struct Hook *h, void *dummy, IPTR *funcptr)
{
    void (*func) (IPTR *) = (void (*)(IPTR *)) (*funcptr);
    if (func) func(funcptr + 1);
}

/****************************************************************
 The display function for the page listview
*****************************************************************/
static void main_page_list_display(struct Hook *h, char **strings, struct page_entry *entry)
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
    int new_active = XGET(main_page_list,MUIA_List_Active);
    Object *new_group;

    if (new_active == -1)
	new_group = main_page_space;
    else
    {
	new_group = main_page_entries[new_active].group;
	if (!new_group)
	    new_group = main_page_space;
    }

    if (new_group == main_page_group_displayed)
	return;

    DoMethod(main_page_group, MUIM_Group_InitChange);
    DoMethod(main_page_group, OM_REMMEMBER, (IPTR)main_page_group_displayed);
    DoMethod(main_page_group, OM_ADDMEMBER, (IPTR)new_group);
    DoMethod(main_page_group, MUIM_Group_ExitChange);
    main_page_group_displayed = new_group;
}

/****************************************************************
 Save pressed
*****************************************************************/
void main_save_pressed(void)
{
    save_prefs(appname, TRUE);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/****************************************************************
 Use pressed
*****************************************************************/
void main_use_pressed(void)
{
    save_prefs(appname, FALSE);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/****************************************************************
 Test pressed
*****************************************************************/
void main_test_pressed(void)
{
    test_prefs();
}

/****************************************************************
 Cancel pressed
*****************************************************************/
void main_cancel_pressed(void)
{
    restore_prefs(appname);
    DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

/****************************************************************
 Look for MCPs
 
 TODO: Write for AmigaOS4
*****************************************************************/
void find_mcps(void)
{
#ifndef __amigaos4__
    static CONST_STRPTR const searchpaths[] =
    {
        "Zune/#?.(mcp|mcc)",
        "Classes/Zune/#?.(mcp|mcc)",
        "MUI/#?.(mcp|mcc)",
        NULL,
    };
    CONST_STRPTR const *pathptr;
    struct DevProc *dp = NULL;
    struct page_entry *pe;
    WORD   num_page_entries = 0;
    BPTR olddir;
    
    for(pe = main_page_entries; pe->name; pe++)
    {
    	num_page_entries++;
    }
    
    olddir = CurrentDir(BNULL);
    
    while((dp = GetDeviceProc("LIBS:", dp)))
    {
    	CurrentDir(dp->dvp_Lock);
	
	for(pathptr = searchpaths; *pathptr; pathptr++)
	{
	    struct AnchorPath *ap;
	    LONG    	       match;
	    
	    ap = (struct AnchorPath *)AllocVec(sizeof(struct AnchorPath) + 256, MEMF_CLEAR);
	    if (ap)
	    {
	    	ap->ap_Strlen = 256;
		
		for(match = MatchFirst((STRPTR)*pathptr, ap);
		    match == 0;
		    match = MatchNext(ap))
		{
		    struct Library *mcclib;
		    
		    if (num_page_entries < MAX_PAGE_ENTRIES)
		    {
			if ((mcclib = OpenLibrary(ap->ap_Buf, 0)))
			{
		    	    struct MUI_CustomClass *mcp;

		    	    if ((mcp = MCC_Query(1)))
			    {
			    	char *sp;
				
				pe->cl = mcp;
				pe->mcp_library = mcclib;
				mcclib = NULL;
				
				pe->mcp_namebuffer[0] = 27;
				pe->mcp_namebuffer[1] = '3';
				strncpy(pe->mcp_namebuffer + 2, mcp->mcc_Class->cl_ID, sizeof(pe->mcp_namebuffer) - 3);
				
				if ((sp = strrchr(pe->mcp_namebuffer, '.')))
				    *sp = '\0';
				
				pe->name = pe->mcp_namebuffer;
						
				pe++;
				num_page_entries++;
				
			    } /* if ((mcp = MCC_Query(1))) */

		    	    if (mcclib) CloseLibrary(mcclib);
			    
			} /* if ((mcclib = OpenLibrary(ap->ap_Buf, 0))) */

		    } /* if (num_page_entries < MAX_PAGE_ENTRIES) */
		    
		} /* for(match = ... */
		
		MatchEnd(ap);
		
	    	FreeVec(ap);
		
	    } /* if (ap) */
	    
	} /* for(pathptr = searchpaths; *pathptr; pathptr++) */
	
    	if (!(dp->dvp_Flags & DVPF_ASSIGN)) break;
	
    } /* while((dp = GetDeviceProc("LIBS:", dp))) */
    
    FreeDeviceProc(dp);
    
    CurrentDir(olddir);
#endif
}

/****************************************************************
 Deallocates all gui resources
*****************************************************************/
void deinit_gui(void)
{
    int i;

    if (app)
	MUI_DisposeObject(app);

    for (i = 0; main_page_entries[i].name; i++)
    {
	if ((main_page_entries[i].group != NULL) &&
	    (main_page_entries[i].group != main_page_group_displayed))
	{
	    DisposeObject(main_page_entries[i].group);
	}

    	if (main_page_entries[i].mcp_library)
	{
	    main_page_entries[i].cl = NULL; /* Prevent MUI_DeleteCustomClass call below */
	   
	    if ((main_page_entries[i].group == NULL) ||
	        (main_page_entries[i].group != main_page_group_displayed))
	    {
	    	/* Only close library if main_page_group_displayed is not this page,
		   because in that case the object got automatically killed through
		   MUI_DisposeObject(app) which also does the CloseLibrary()!! */
		   
    		CloseLibrary(main_page_entries[i].mcp_library);
    		main_page_entries[i].mcp_library = NULL;
	    }	    
	}
	
	main_page_entries[i].group = NULL;
    	
	if (main_page_entries[i].cl != NULL)
	{
	    MUI_DeleteCustomClass(main_page_entries[i].cl);
	    main_page_entries[i].cl = NULL;
	}
    }

    if (main_page_group_displayed != main_page_space)
	MUI_DisposeObject(main_page_space);
}

/****************************************************************
 Allocate resources for gui
*****************************************************************/
int init_gui(void)
{
    Object *save_button;
    Object *use_button;
    Object *test_button;
    Object *cancel_button;
    STRPTR wintitle;
    char titlebuf[255];
    
    main_page_entries[ 0].name = (char *)_(MSG_DSC_SYSTEM);
    main_page_entries[ 1].name = (char *)_(MSG_DSC_WINDOWS);
    main_page_entries[ 2].name = (char *)_(MSG_DSC_GROUPS);
    main_page_entries[ 3].name = (char *)_(MSG_DSC_BUTTONS);
    main_page_entries[ 4].name = (char *)_(MSG_DSC_CYCLES);
    main_page_entries[ 5].name = (char *)_(MSG_DSC_SLIDERS);
    main_page_entries[ 6].name = (char *)_(MSG_DSC_SCROLLBARS);
    main_page_entries[ 7].name = (char *)_(MSG_DSC_LISTVIEWS);
    main_page_entries[ 8].name = (char *)_(MSG_DSC_STRINGS);
    main_page_entries[ 9].name = (char *)_(MSG_DSC_NAVIGATION);
    main_page_entries[10].name = (char *)_(MSG_DSC_SPECIAL);
    main_page_entries[11].name = (char *)_(MSG_DSC_FRAMES);

    static struct Hook page_display_hook;

#ifdef __amigaos4__
    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    page_display_hook.h_Entry = (HOOKFUNC)main_page_list_display;
#else
    hook_standard.h_Entry = HookEntry;
    hook_standard.h_SubEntry = (APTR)hook_func_standard;
    page_display_hook.h_Entry = HookEntry;
    page_display_hook.h_SubEntry = (APTR)main_page_list_display;
#endif

    if (!strcmp(appname, "global"))
	wintitle = (STRPTR) _(MSG_WINTITLE1);
    else
    {
	snprintf(titlebuf, 255, _(MSG_WINTITLE2), appname);
	wintitle = titlebuf;
    }

    app = ApplicationObject,
        MUIA_Application_Title, (IPTR) "Zune",
        MUIA_Application_Version, (IPTR) ZUNEVERSION,
        MUIA_Application_Copyright, (IPTR) " 2006, The AROS Development Team",
        MUIA_Application_Description, wintitle,
        MUIA_Application_Base, "ZUNEPREF",
	MUIA_Application_Menustrip, MenuitemObject,
	    MUIA_Family_Child, MenuitemObject,
	    	MUIA_Menuitem_Title, _(MSG_MEN_PROJECT),
	    	MUIA_Family_Child, open_menuitem = MenuitemObject,
		    MUIA_Menuitem_Title, _(MSG_MEN_OPEN), MUIA_Menuitem_Shortcut, _(MSG_MEN_OPEN_SC),
		End,
	    	MUIA_Family_Child, append_menuitem = MenuitemObject, MUIA_Menuitem_Title, _(MSG_MEN_APPEND), End,
	    	MUIA_Family_Child, saveas_menuitem = MenuitemObject,
		    MUIA_Menuitem_Title, _(MSG_MEN_SAVEAS), MUIA_Menuitem_Shortcut, _(MSG_MEN_SAVEAS_SC),
		End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, NM_BARLABEL, End,
	    	MUIA_Family_Child, aboutzune_menuitem = MenuitemObject,
		    MUIA_Menuitem_Title, _(MSG_MEN_ABOUT), MUIA_Menuitem_Shortcut, _(MSG_MEN_ABOUT_SC),
		End,
	    	MUIA_Family_Child, MenuitemObject, MUIA_Menuitem_Title, NM_BARLABEL, End,
	    	MUIA_Family_Child, quit_menuitem = MenuitemObject,
		    MUIA_Menuitem_Title, _(MSG_MEN_QUIT), MUIA_Menuitem_Shortcut, _(MSG_MEN_QUIT_SC),
		End,
	    	End,
	    End,
    	SubWindow, main_wnd = WindowObject,
    	    MUIA_Window_Title, (IPTR)wintitle,
	    MUIA_Window_Activate, TRUE,
	    MUIA_Window_CloseGadget, FALSE,
	    MUIA_Window_ID, MAKE_ID('Z','W','I','N'),

	    WindowContents, VGroup,
	        MUIA_Group_VertSpacing, 10,
    	    	Child, HGroup,
	          Child, VGroup,
		    Child, (NewObject(ClassListview_CLASS->mcc_Class, NULL,
	                MUIA_CycleChain, 1,
			MUIA_Listview_List, main_page_list = ListObject,
			    InputListFrame,
	                    MUIA_List_AdjustWidth, TRUE,
			    MUIA_List_DisplayHook, &page_display_hook,
			    End,
			TAG_DONE)),
		    Child, HGroup,
	                Child, MUI_NewObject(MUIC_Popframe,
					     MUIA_FixHeight, 20,
					     MUIA_Window_Title, (IPTR) _(MSG_FRAME_CLIPBOARD),
					     TAG_DONE),
	                Child, MUI_NewObject(MUIC_Popimage,
					     MUIA_FixHeight, 20,
					     MUIA_Imageadjust_Type, MUIV_Imageadjust_Type_All,
					     MUIA_Window_Title, (IPTR) _(MSG_IMAGE_CLIPBOARD),
					     TAG_DONE),
		        End, /* HGroup */
	            End,
		    Child, VGroup,
	                TextFrame,
			InnerSpacing(6,6),
	                MUIA_Background, MUII_PageBack,
			Child, main_page_group = VGroup,
			    Child, main_page_group_displayed = main_page_space = HVSpace,
			    End,
			End,
		    End,
		Child, HGroup,
		    Child, test_button = ImageButton(_(MSG_GAD_TEST), "THEME:Images/Gadgets/Prefs/Test"),
		    Child, HVSpace,
		    Child, save_button = ImageButton(_(MSG_GAD_SAVE), "THEME:Images/Gadgets/Prefs/Save"),
		    Child, use_button = ImageButton(_(MSG_GAD_USE), "THEME:Images/Gadgets/Prefs/Use"),
		    Child, cancel_button = ImageButton(_(MSG_GAD_CANCEL), "THEME:Images/Gadgets/Prefs/Cancel"),
		    End,
		End,
    	    End,
    	End;

    if (app)
    {
    	int i;

	DoMethod(main_wnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		 (IPTR)app, 3, MUIM_CallHook, (IPTR)&hook_standard,
		 (IPTR)main_cancel_pressed);
	DoMethod(cancel_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 3,
		 MUIM_CallHook, (IPTR)&hook_standard, (IPTR)main_cancel_pressed);
	DoMethod(save_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 3,
		 MUIM_CallHook, (IPTR)&hook_standard, (IPTR)main_save_pressed);
	DoMethod(use_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 3,
		 MUIM_CallHook, (IPTR)&hook_standard, (IPTR)main_use_pressed);
	DoMethod(test_button, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)app, 6,
		 MUIM_Application_PushMethod, (IPTR)app, 3, MUIM_CallHook,
		 (IPTR)&hook_standard, (IPTR)main_test_pressed);
	DoMethod(quit_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger,
		 MUIV_EveryTime, (IPTR)app, 3, MUIM_CallHook,
		 (IPTR)&hook_standard, (IPTR)main_cancel_pressed);
        DoMethod(open_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger,
                 MUIV_EveryTime, (IPTR)app, 3, MUIM_CallHook,
                 (IPTR)&hook_standard, (IPTR)main_open_menu);   
        DoMethod(saveas_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger,
                 MUIV_EveryTime, (IPTR)app, 3, MUIM_CallHook,
                 (IPTR)&hook_standard, (IPTR)main_saveas_menu); 
	DoMethod(aboutzune_menuitem, MUIM_Notify, MUIA_Menuitem_Trigger,
		 MUIV_EveryTime, (IPTR)app, 2, MUIM_Application_AboutMUI,
		 (IPTR)main_wnd);

	for (i = 0; main_page_entries[i].name != NULL; i++)
	{
	    struct page_entry *p = &main_page_entries[i];

	    if (!p->cl) p->cl = create_class(p->desc);

	    if (!(p->cl && (p->group = NewObject(p->cl->mcc_Class, NULL, TAG_DONE))))
	    {
		deinit_gui();
		return 0;
	    }

	    DoMethod(main_page_list, MUIM_List_InsertSingle, (IPTR)p,
		     MUIV_List_Insert_Bottom);
	}

	DoMethod(main_page_list, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		 (IPTR)app, 3, MUIM_CallHook, (IPTR)&hook_standard,
		 (IPTR)main_page_active);

	/* Activate first entry */
	set(main_page_list,MUIA_List_Active,0);

	return 1;
    }
    return 0;
}

/****************************************************************
 Load the given prefs
*****************************************************************/
void load_prefs(CONST_STRPTR name)
{
    Object *configdata;

    configdata = MUI_NewObject(MUIC_Configdata,
			       MUIA_Configdata_ApplicationBase, (IPTR)name,
			       TAG_DONE);
    if (configdata != NULL)
    {
	int i;

/*  	D(bug("zune::load_prefs: created configdata %p\n", configdata)); */
	LastSavedConfigdata = configdata;

        /* Call MUIM_Settingsgroup_ConfigToGadgets for every group */
	for (i=0;main_page_entries[i].name;i++)
	{
	    struct page_entry *p = &main_page_entries[i];
	    if (p->group)
		DoMethod(p->group, MUIM_Settingsgroup_ConfigToGadgets,
			 (IPTR)configdata);
	}

/*  	D(bug("zune::save_prefs: disposed configdata %p\n", configdata)); */
    }
}

/* write prefs to env: */
void test_prefs(void)
{
    Object *cfg;

    save_prefs(appname, FALSE);
/*      load_prefs(); */
    cfg = MUI_NewObject(MUIC_Configdata, MUIA_Configdata_Application, (IPTR)app, TAG_DONE);
    set(app, MUIA_Application_Configdata, (IPTR)cfg);
}

void restore_prefs(CONST_STRPTR name)
{
    char buf[255];

    snprintf(buf, 255, "ENV:zune/%s.prefs", name);
    DoMethod(LastSavedConfigdata, MUIM_Configdata_Save, (IPTR)buf);
}


/****************************************************************
 open given prefs file from menue
*****************************************************************/

void main_open_menu(void)
{
    static char dirpart[500]="ENVARC:Zune",filepart[500],filename[1000];

    if (aslfilerequest ("Load a Zune Prefs File", (char *)&dirpart, (char *)&filepart, (char *)&filename, prefstags));
    {   
        Object *configdata;

        configdata = MUI_NewObject(MUIC_Configdata,
                     MUIA_Configdata_ApplicationBase,appname,
                     TAG_DONE);

        if (configdata != NULL)
        {
            int i;

            /*      D(bug("zune::load_prefs: created configdata %p\n", configdata)); */
            //LastSavedConfigdata = configdata;
            DoMethod(configdata, MUIM_Configdata_Load,filename);

            /* Call MUIM_Settingsgroup_ConfigToGadgets for every group */           
            for (i=0;main_page_entries[i].name;i++)
            {
                struct page_entry *p = &main_page_entries[i];
                
                if (p->group) DoMethod(p->group, MUIM_Settingsgroup_ConfigToGadgets, (IPTR)configdata);
            }

            /* activate prefs in test mode */
            test_prefs();

            /*      D(bug("zune::save_prefs: disposed configdata %p\n", configdata)); */
        }                     
                          
    } 
}

/****************************************************************
 Saves the done prefs
*****************************************************************/
void save_prefs(CONST_STRPTR name, BOOL envarc)
{
    Object *configdata;

    configdata = MUI_NewObject(MUIC_Configdata,
                    //MUIA_Configdata_ApplicationBase, name,
			       MUIA_Configdata_Application,appaddr,
			       TAG_DONE);
    if (configdata != NULL)
    {
	int i;
	char buf[255];

/*  	D(bug("zune::save_prefs: created configdata %p\n", configdata)); */

        /* Call MUIM_Settingsgroup_GadgetsToConfig for every group */
	for (i=0;main_page_entries[i].name;i++)
	{
	    struct page_entry *p = &main_page_entries[i];
	    if (p->group)
		DoMethod(p->group, MUIM_Settingsgroup_GadgetsToConfig,
			 (IPTR)configdata);
	}

	if (envarc)
	{
	    snprintf(buf, 255, "ENVARC:zune/%s.prefs", name);
	    DoMethod(configdata, MUIM_Configdata_Save, (IPTR)buf);
	}
	snprintf(buf, 255, "ENV:zune/%s.prefs", name);
	DoMethod(configdata, MUIM_Configdata_Save, (IPTR)buf);

    	MUI_DisposeObject(configdata);
/*  	D(bug("zune::save_prefs: disposed configdata %p\n", configdata)); */
    }
}


/****************************************************************
 SaveAS the done prefs from menue
*****************************************************************/

void main_saveas_menu(void)
{
    static char dirpart[500]="ENVARC:Zune",filepart[500],filename[1000];

    if (aslfilerequest("Save a Zune Prefs File", (char *)&dirpart, (char *)&filepart, (char *)&filename, prefstags));
    {   
        Object *configdata;

        configdata = MUI_NewObject(MUIC_Configdata,
                     MUIA_Configdata_ApplicationBase, "dummyfile",
                     TAG_DONE);

        /* check for ".prefs" suffix in filename, add if not existing */
        if ( !strstr( filename, ".prefs") ) strcat(filename, ".prefs");

        if (configdata != NULL)
        {
            int i;

            /*      D(bug("zune::save_prefs: created configdata %p\n", configdata)); */

            /* Call MUIM_Settingsgroup_GadgetsToConfig for every group */    
            for (i=0;main_page_entries[i].name;i++)
            {
                struct page_entry *p = &main_page_entries[i];
        
                if (p->group) DoMethod(p->group, MUIM_Settingsgroup_GadgetsToConfig, (IPTR)configdata);
            }

            DoMethod(configdata, MUIM_Configdata_Save, filename);

            MUI_DisposeObject(configdata);
            /*      D(bug("zune::save_prefs: disposed configdata %p\n", configdata)); */
        }                     
                                  
    } 

}


/****************************************************************
 The message loop
*****************************************************************/
void loop(void)
{
    ULONG sigs = 0;

    while((LONG) DoMethod(app, MUIM_Application_NewInput, (IPTR)&sigs)
	  != MUIV_Application_ReturnID_Quit)
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

int main(void)
{
    BPTR OldDir = BNULL, NewDir;
    int  retval = RETURN_OK;
    struct RDArgs *rda = NULL;
//    APTR *proc=0;
    IPTR args[] = { 0,0 };
    enum { ARG_APPNAME = 0,ARG_APPADDR=1 };

    Locale_Initialize();

    if (Cli())
    {
    rda = ReadArgs("/A,/N", args, NULL);
    appname=(STRPTR)args[ARG_APPNAME];
    appaddr=(APTR)args[ARG_APPADDR];
    if (appaddr)appaddr=*(appaddr);
	}
	else
	{
	    appname = 0;
	}

    
    if (retval == RETURN_OK)
    {
	if (!appname)
	    appname = "global";

	if (open_libs())
	{
	    if (open_classes())
	    {
	    	find_mcps();
		NewDir = Lock("RAM:", SHARED_LOCK);
		if (NewDir)
		    OldDir = CurrentDir(NewDir);
		if (init_gui())
		{
		    load_prefs((STRPTR)args[ARG_APPNAME]);
		    set(main_wnd, MUIA_Window_Open, TRUE);
		    if (XGET(main_wnd,MUIA_Window_Open))
		    {
			loop();
		    }
		    if (LastSavedConfigdata)
			MUI_DisposeObject(LastSavedConfigdata);
		    deinit_gui();
		}
		if (NewDir) {
		    CurrentDir(OldDir);
		    UnLock(NewDir);
		}
		close_classes();
	    }
	    close_libs();
	}
    }
    
    if (rda) FreeArgs(rda);

    Locale_Deinitialize();
    return retval;
}
