/*
    Copyright � 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <dos/notify.h>
#include <workbench/handler.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/workbench.h>

#include <string.h>
#include <stdio.h>

#define DEBUG 1
#include <aros/debug.h>

#include "iconwindow.h"
#include "wandererprefs.h"
#include "wanderer.h"

VOID DoAllMenuNotifies(Object *strip, char *path);
Object *FindMenuitem(Object* strip, int id);
VOID DoDetach(VOID);

extern Object *app;
extern Object *root_iconwnd;
extern Object *root_menustrip;
struct Hook hook_standard;
struct Hook hook_action;


char *GetScreenTitle(void)
{
    static char title[256];
    /* AROS probably don't have graphics mem but without it look so empty */
    sprintf(title,"Wanderer  %ld graphics mem  %ld other mem",AvailMem(MEMF_CHIP),AvailMem(MEMF_FAST));
    return title;
}

enum
{
    MEN_WANDERER = 1,
    MEN_WANDERER_BACKDROP,
    MEN_WANDERER_EXECUTE,
    MEN_WANDERER_SHELL,
    MEN_WANDERER_GUISETTINGS,
    MEN_WANDERER_ABOUT,
    MEN_WANDERER_QUIT,
    MEN_ICON_OPEN,
    MEN_ICON_INFORMATION,
    MEN_ICON_DELETE,
};

static struct NewMenu nm[] =
{
  {NM_TITLE, "Wanderer"              },
    {NM_ITEM,  "Backdrop",           "B", CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WANDERER_BACKDROP},
    {NM_ITEM,  "Execute Command...", "E", 0,               0, (APTR) MEN_WANDERER_EXECUTE},
    /*{NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },*/
    {NM_ITEM,  "Shell",              "W", 0,               0, (APTR) MEN_WANDERER_SHELL},
    {NM_ITEM,  "GUI Settings...",        NULL, 0,          0, (APTR) MEN_WANDERER_GUISETTINGS},
    {NM_ITEM,  "About...",           "?", 0,               0, (APTR) MEN_WANDERER_ABOUT},
    {NM_ITEM,  "Quit...",            "Q", 0,               0, (APTR) MEN_WANDERER_QUIT},

  /*{NM_TITLE, "Window",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "New Drawer", "N"},
    {NM_ITEM,  "Open Parent" },
    {NM_ITEM,  "Close", "K"},
    {NM_ITEM,  "Update" },
    {NM_ITEM,  "Select contents", "A"},
    {NM_ITEM,  "Clear selection", "Z"},
    {NM_ITEM,  "Clean Up", "."},
    {NM_ITEM,  "Snapshot" },
      {NM_SUB, "Window"},
      {NM_SUB, "All"},
    {NM_ITEM,  "Show" },
      {NM_SUB, "Only Icons", NULL, CHECKIT | CHECKED, 2},
      {NM_SUB, "All Files", NULL, CHECKIT, 1 },
    {NM_ITEM,  "View By" },
      {NM_SUB, "Icon", NULL, CHECKIT | CHECKED, 2 + 4 + 8},
      {NM_SUB, "Name",NULL, CHECKIT, 1 + 4 + 8},
      {NM_SUB, "Size",NULL, CHECKIT, 1 + 2 + 8},
      {NM_SUB, "Date", NULL, CHECKIT, 1 + 2 + 4},*/

  {NM_TITLE, "Icon",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "Open", "O", 0, 0, (APTR) MEN_ICON_OPEN},
/*    {NM_ITEM,  "Close","C" },
    {NM_ITEM,  "Rename...", "R"},*/
    {NM_ITEM,  "Information...", "I", 0, 0, (APTR) MEN_ICON_INFORMATION},
/*    {NM_ITEM,  "Snapshot", "S" },
    {NM_ITEM,  "Unsnapshot", "U" },
    {NM_ITEM,  "Leave Out", "L" },
    {NM_ITEM,  "Put Away", "P" },*/
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM,  "Delete...", NULL, 0, 0, (APTR) MEN_ICON_DELETE},
/*    {NM_ITEM,  "Format Disk..." },
    {NM_ITEM,  "Empty Trash..." },

  {NM_TITLE, "Tools",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "ResetWanderer" },
  {NM_END}*/

};

/**************************************************************************
 Open the execute window. Simliar to above but you can also set the
 command. Called when item is openend
**************************************************************************/
void execute_open_with_command(BPTR cd, char *contents)
{
    BPTR lock;
    
    if (cd != NULL) lock = cd;
    else            lock = Lock("RAM:", ACCESS_READ);
        
    OpenWorkbenchObject
    (
        "WANDERER:Tools/ExecuteCommand",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, (IPTR) contents,
        TAG_DONE
    );
    
    if (cd == NULL) UnLock(lock);
}

/**************************************************************************
 Open the execute window

 This function will always get the current drawer as argument
**************************************************************************/
VOID execute_open(STRPTR *cdptr)
{
    execute_open_with_command(cdptr != NULL ? Lock(*cdptr, SHARED_LOCK) : NULL, NULL);
}

/*******************************/

void shell_open(char **cd_ptr)
{
    BPTR cd = Lock(*cd_ptr,ACCESS_READ);

    if (SystemTags("NewShell", NP_CurrentDir, (IPTR)cd, TAG_DONE) == -1)
    {
    	UnLock(cd);
    }
}

void wanderer_backdrop(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip,MEN_WANDERER_BACKDROP);
    if (item)
    {
    	set(root_iconwnd,MUIA_Window_Open,FALSE);
	set(root_iconwnd,MUIA_IconWindow_IsBackdrop, XGET(item, MUIA_Menuitem_Checked));
    	set(root_iconwnd,MUIA_Window_Open,TRUE);
    }
}

void icon_open()
{
    struct List *windowList = NULL;
    
    if (GET(app, MUIA_Application_WindowList, (IPTR *) &windowList))
    {
        Object *lstate = (Object *) windowList->lh_Head;
        Object *window = NULL;
        
        while ((window = NextObject(&lstate)) != NULL)
        {
            if
            (
                   XGET(window, MUIA_UserData) 
                && XGET(window, MUIA_Window_Activate)
            )
            {
                DoMethod(window, MUIM_IconWindow_DoubleClicked);
                break;
            }
        }
    }
}

void icon_information()
{
    struct List *windowList = NULL;
        
    if (GET(app, MUIA_Application_WindowList, (IPTR *) &windowList))
    {
        Object *lstate = (Object *) windowList->lh_Head;
        Object *window = NULL;
        
        while ((window = NextObject(&lstate)) != NULL)
        {
            if
            (
                   XGET(window, MUIA_UserData) 
                && XGET(window, MUIA_Window_Activate)
            )
            {
                Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
                struct IconList_Entry *entry   = (void*) MUIV_IconList_NextSelected_Start;
                kprintf("*** found active window\n");
                
                do
                {
                    DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
                    if ((int)entry == MUIV_IconList_NextSelected_End) break;
                    kprintf("*** selected: %s\n", entry->filename);
                    
                    {
                        BPTR lock   = Lock(entry->filename, ACCESS_READ);
                        BPTR parent = ParentDir(lock);
                        
                        WBInfo(parent, FilePart(entry->filename), NULL);
                        
                        kprintf("*** selected: %s\n", entry->filename);
                    
                        UnLock(parent);
                        UnLock(lock);
                    }
                } while (TRUE);
                
                break;
            }
        }
    }
}

void icon_delete(void)
{
    struct List *windowList = NULL;
        
    if (GET(app, MUIA_Application_WindowList, (IPTR *) &windowList))
    {
        Object *lstate = (Object *) windowList->lh_Head;
        Object *window = NULL;
        
        while ((window = NextObject(&lstate)) != NULL)
        {
            if
            (
                   XGET(window, MUIA_UserData) 
                && XGET(window, MUIA_Window_Activate)
            )
            {
                Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
                struct IconList_Entry *entry   = (void*) MUIV_IconList_NextSelected_Start;
                kprintf("*** found active window\n");
                
                do
                {
                    DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
                    if ((int)entry == MUIV_IconList_NextSelected_End) break;
                    kprintf("*** selected: %s\n", entry->filename);
                    
                    {
                        BPTR lock   = Lock(entry->filename, ACCESS_READ);
                        BPTR parent = ParentDir(lock);
                        UnLock(lock);
                        
                        OpenWorkbenchObject
                        (
                            "WANDERER:Tools/Delete",
                            WBOPENA_ArgLock, (IPTR) parent,
                            WBOPENA_ArgName, (IPTR) FilePart(entry->filename),
                            TAG_DONE
                        );
                        
                        kprintf("*** selected: %s\n", entry->filename);
                    
                        UnLock(parent);
                    }
                } while (TRUE);
                
                break;
            }
        }
    }
}

void wanderer_guisettings(void)
{
    DoMethod(app, MUIM_Application_OpenConfigWindow);
}

void wanderer_about(void)
{
    OpenWorkbenchObject("SYS:System/About", TAG_DONE);
}

void wanderer_quit(void)
{
    if (MUI_RequestA(app,NULL,0,"Wanderer", "*Ok|Cancel", "Do you really want to quit Wanderer?",NULL))
	DoMethod(app, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
}

AROS_UFH3(void, hook_func_action,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1))
{
    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
	static char buf[1024];
	struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
	Object *drawerwnd;

	DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
	if ((int)ent == MUIV_IconList_NextSelected_End) return;

	if (msg->isroot)
	{
	    strcpy(buf,ent->label);
	    strcat(buf,":");
	} else
	{
	    strcpy(buf,ent->filename);
	}

	if (ent->type == ST_ROOT || ent->type == ST_USERDIR)
	{
	    Object *cstate = (Object*)(((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
	    Object *child;

	    while ((child = NextObject(&cstate)))
	    {
	    	if (XGET(child, MUIA_UserData))
	    	{
		    char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
		    if (child_drawer && !Stricmp(buf,child_drawer))
		    {
		    	int is_open = XGET(child, MUIA_Window_Open);
		    	if (!is_open)
			    DoMethod(child, MUIM_IconWindow_Open);
			else
			{
			    DoMethod(child, MUIM_Window_ToFront);
			    set(child, MUIA_Window_Activate, TRUE);
			}
		    	return;
		    }
	    	}
	    }

	    {
		/* Check if the window for this drawer is already opened */
		Object *menustrip;

		/* Create a new icon drawer window with the correct drawer being set */
		drawerwnd = IconWindowObject,
		    MUIA_UserData, 1,
		    MUIA_Window_Menustrip, (IPTR) menustrip = MUI_MakeObject(MUIO_MenustripNM, (IPTR) nm, (IPTR) NULL),
		    MUIA_IconWindow_IsRoot, FALSE,
		    MUIA_IconWindow_ActionHook, (IPTR) &hook_action,
		    MUIA_IconWindow_Drawer, (IPTR) buf,
                End;

		if (drawerwnd)
		{
	    	    /* Get the drawer path back so we can use it also outside this function */
		    char *drw = (char*)XGET(drawerwnd,MUIA_IconWindow_Drawer);

		    /* We simply close the window here in case somebody like to to this...
		     * the memory is not freed until wb is closed however */
		    DoMethod
                    (
                        drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
                        (IPTR) drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE
                    );

		    /* If "Execute Command" entry is clicked open the execute window */
		    DoAllMenuNotifies(menustrip,drw);

		    /* Add the window to the application */
		    DoMethod(app, OM_ADDMEMBER, (IPTR) drawerwnd);

		    /* And now open it */
		    DoMethod(drawerwnd, MUIM_IconWindow_Open);
		}
	    }
	} 
        else if (ent->type == ST_FILE)
	{
	    BPTR newwd, oldwd, file;

	    /* Set the CurrentDir to the path of the executable to be started */
	    file = Lock(ent->filename, SHARED_LOCK);
	    if(file)
	    {
		newwd = ParentDir(file);
		oldwd = CurrentDir(newwd);
		if (!OpenWorkbenchObject(ent->filename, TAG_DONE))
		{
		    execute_open_with_command(newwd, FilePart(ent->filename));
		}
		CurrentDir(oldwd);
		UnLock(newwd);
		UnLock(file);
	    }
	}
    } 
    else if (msg->type == ICONWINDOW_ACTION_CLICK)
    {
	if (!msg->click->shift)
	{
	    Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
	    Object *child;

	    while ((child = NextObject(&cstate)))
	    {
	    	if (XGET(child, MUIA_UserData))
	    	{
		    if (child != obj)
		    {
			DoMethod(child, MUIM_IconWindow_UnselectAll);
		    }
		}
	    }
	}
    } 
    else if (msg->type == ICONWINDOW_ACTION_ICONDROP)
    {
	Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
	Object *child;

	while ((child = NextObject(&cstate)))
	{
	    if (XGET(child, MUIA_UserData))
	    {
		struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
		Object *iconlist = (Object*)XGET(child, MUIA_IconWindow_IconList);

		do
		{
		    DoMethod(iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
		    if ((int)ent == MUIV_IconList_NextSelected_End) break;

		    Printf("%s\n",ent->filename);
		} while(1);
	    }
	}
    }
}



AROS_UFH3
(
    void, hook_func_standard,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);

    if (func) func((ULONG *)(funcptr + 1));
}

/**************************************************************************
 This function returns a Menu Object with the given id
**************************************************************************/
Object *FindMenuitem(Object* strip, int id)
{
    return (Object*)DoMethod(strip, MUIM_FindUData, id);
}

/**************************************************************************
 This connects a notify to the given menu entry id
**************************************************************************/
VOID DoMenuNotify(Object* strip, int id, void *function, void *arg)
{
    Object *entry;
    entry = FindMenuitem(strip,id);
    if (entry)
    {
	DoMethod
        (
            entry, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            (IPTR) entry, 4, MUIM_CallHook, (IPTR) &hook_standard,
            (IPTR) function, (IPTR) arg
        );
    }
}

VOID DoAllMenuNotifies(Object *strip, char *path)
{
    Object *item;

    if (!strip) return;

    DoMenuNotify(strip,MEN_WANDERER_EXECUTE,execute_open, path);
    DoMenuNotify(strip,MEN_WANDERER_SHELL,shell_open,path);
    DoMenuNotify(strip,MEN_WANDERER_GUISETTINGS,wanderer_guisettings,NULL);
    DoMenuNotify(strip,MEN_WANDERER_ABOUT,wanderer_about,NULL);
    DoMenuNotify(strip,MEN_WANDERER_QUIT,wanderer_quit,NULL);
    DoMenuNotify(strip,MEN_ICON_OPEN,icon_open,NULL);
    DoMenuNotify(strip,MEN_ICON_INFORMATION,icon_information,NULL);
    DoMenuNotify(strip,MEN_ICON_DELETE,icon_delete,NULL);
    
    if ((item = FindMenuitem(strip,MEN_WANDERER_BACKDROP)))
    {
	DoMethod
        (
            item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            (IPTR) app, 7, MUIM_Application_PushMethod, 
            (IPTR) app, 4, MUIM_CallHook, (IPTR) &hook_standard, 
            (IPTR) wanderer_backdrop, (IPTR) strip
        );
    }
}

/*** Instance Data **********************************************************/
struct Wanderer_DATA
{
    Object *wd_Prefs;

    struct MUI_InputHandlerNode timer_ihn;
    struct MUI_InputHandlerNode notify_ihn;
    struct MsgPort *notify_port;
    
    struct NotifyRequest         pnr;
    struct MsgPort              *pnotify_port;
    struct MUI_InputHandlerNode  pnotify_ihn;
};

/*** Macros *****************************************************************/
#define SETUP_INST_DATA struct Wanderer_DATA *data = INST_DATA(CLASS, self)

/*** Methods ****************************************************************/
Object *Wanderer__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title,       (IPTR) "Wanderer",
	MUIA_Application_Base,        (IPTR) "WANDERER",
	MUIA_Application_Version,     (IPTR) "$VER: Wanderer 0.1 (10.12.02)", // FIXME
	MUIA_Application_Description, (IPTR) "File manager",
	MUIA_Application_SingleTask,         TRUE,
    	
        SubWindow, (IPTR) root_iconwnd = IconWindowObject,
	    MUIA_UserData, 1,
	    MUIA_Window_Menustrip,      (IPTR) root_menustrip = MUI_MakeObject(MUIO_MenustripNM, (IPTR) nm, (IPTR) NULL),
	    MUIA_Window_ScreenTitle,    (IPTR) GetScreenTitle(),
            MUIA_IconWindow_IsRoot,            TRUE,
	    MUIA_IconWindow_IsBackdrop,        TRUE,
	    MUIA_IconWindow_ActionHook, (IPTR) &hook_action,
        End,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;
        hook_action.h_Entry   = (HOOKFUNC) hook_func_action;

        if (!(data->notify_port = CreateMsgPort()))
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
        D(bug("Wanderer: notify port: %p\n", data->notify_port));
    
        if ((data->pnotify_port = CreateMsgPort()) == NULL)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
    
        RegisterWorkbench(data->notify_port);
    
        /* Setup three input handlers */
    
        /* The first one is invoked every time we get a message from the Notify Port */
        data->notify_ihn.ihn_Signals = 1UL<<data->notify_port->mp_SigBit;
        bug("Wanderer: notify signal %ld\n", data->notify_ihn.ihn_Signals);
        data->notify_ihn.ihn_Object = self;
        data->notify_ihn.ihn_Method = MUIM_Wanderer_HandleCommand;
        DoMethod(self, MUIM_Application_AddInputHandler, (IPTR) &data->notify_ihn);
    
    
        /* The second one is a timer handler */
        data->timer_ihn.ihn_Flags = MUIIHNF_TIMER;
        /* called every second (this is only for timer input handlers) */
        data->timer_ihn.ihn_Millis = 3000;
        /* The following method of the given should be called if the
         * event happens */
        data->timer_ihn.ihn_Object = self;
        data->timer_ihn.ihn_Method = MUIM_Wanderer_HandleTimer;
    
        DoMethod(self, MUIM_Application_AddInputHandler, (IPTR) &data->timer_ihn);
    
        /* third one for prefs change notifies */
        data->pnotify_ihn.ihn_Signals = 1UL<<data->pnotify_port->mp_SigBit;
        bug("Wanderer: pnotify signal %ld\n", data->pnotify_ihn.ihn_Signals);
        data->pnotify_ihn.ihn_Object = self;
        data->pnotify_ihn.ihn_Method = MUIM_Wanderer_HandleNotify;
        DoMethod(self, MUIM_Application_AddInputHandler, (IPTR) &data->pnotify_ihn);
    
        data->pnr.nr_Name                 = "ENV:SYS/Wanderer.prefs";
        data->pnr.nr_Flags                = NRF_SEND_MESSAGE;
        data->pnr.nr_stuff.nr_Msg.nr_Port = data->pnotify_port;
        
        if (StartNotify(&data->pnr))
        {
            D(bug("Wanderer: prefs notification setup ok\n"));
        }
        else
        {
            D(bug("Wanderer: prefs notification setup FAILED\n"));
        }
        
        data->wd_Prefs = WandererPrefsObject, End; // FIXME: error handling    
    }
    
    return self;
}

IPTR Wanderer__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_INST_DATA;
    
    if (data->notify_port)
    {
	/*
            They only have been added if the creation of the msg port was
	    successful
        */
	DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->timer_ihn);
	DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->notify_ihn);
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->pnotify_ihn);
	
        UnregisterWorkbench(data->notify_port);
	
        EndNotify(&data->pnr);
        DeleteMsgPort(data->pnotify_port);
        DeleteMsgPort(data->notify_port);
	data->notify_port = NULL;
        
        DisposeObject(data->wd_Prefs);
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

#if 0
IPTR Wanderer__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            /*
            case MUIA_Wanderer_:
                break;
            */
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}
#endif

IPTR Wanderer__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
        case MUIA_Wanderer_Prefs:
            *store = (IPTR) data->wd_Prefs;
            break;
            
        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}

IPTR Wanderer__MUIM_Application_Execute
(
    Class *CLASS, Object *self, Msg message 
)
{
    SETUP_INST_DATA;
    
    DoMethod
    (
        root_iconwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
        (IPTR) app, 3, MUIM_CallHook, (IPTR) &hook_standard, (IPTR) wanderer_quit
    );

    /* If "Execute Command" entry is clicked open the execute window */
    DoAllMenuNotifies(root_menustrip, "RAM:");
    
    /* open root window */
    DoMethod(root_iconwnd, MUIM_IconWindow_Open);
    
    DoDetach();
    
    DoSuperMethodA(CLASS, self, message);
    
    return TRUE;
}

IPTR Wanderer__MUIM_Wanderer_HandleTimer
(
    Class *CLASS, Object *self, Msg message
)
{
    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
    Object *child;
    char *scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	set(child, MUIA_Window_ScreenTitle, (IPTR) scr_title);
    
    return TRUE;
}

IPTR Wanderer__MUIM_Wanderer_HandleCommand
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    struct WBHandlerMessage *wbhm;
    
    D(bug("Wanderer: Recieved signal at notify port\n"));
    
    while ((wbhm = WBHM(GetMsg(data->notify_port))) != NULL)
    {
        D(bug("Wanderer: Recieved message from handler, type = %ld\n", wbhm->wbhm_Type));
        
        switch (wbhm->wbhm_Type)
        {
            case WBHM_TYPE_SHOW:
                #warning WBHM_TYPE_SHOW not supported
                break;
            
            case WBHM_TYPE_HIDE:
                #warning WBHM_TYPE_HIDE not supported
                break;
                
            case WBHM_TYPE_UPDATE:
                D(bug("Wanderer: WBHM_TYPE_UPDATE\n"));
                {
                    CONST_STRPTR name = wbhm->wbhm_Data.Update.Name;
                    ULONG        length;
                    
                    switch (wbhm->wbhm_Data.Update.Type)
                    {
                        case WBDISK:
                        case WBDRAWER:
                        case WBGARBAGE:
                            length = strlen(name);
                            break;
                            
                        default:
                            length = PathPart(name) - name;
                            break;
                    }
                    
                    D(bug("Wanderer: name = %s, length = %ld\n", name, length));
                    
                    {
                        Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                        Object *child;
                        
                        while ((child = NextObject(&cstate)))
                        {
                            if (XGET(child, MUIA_UserData))
                            {
                                char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
                                
                                if
                                (
                                       child_drawer != NULL 
                                    && strncmp(name, child_drawer, length) == 0
                                    && strlen(child_drawer) == length
                                )
                                {
                                    Object *iconlist = (Object *) XGET(child, MUIA_IconWindow_IconList);
                                    
                                    D(bug("Wanderer: Drawer found: %s!\n", child_drawer));
                                    
                                    if (iconlist != NULL)
                                    {
                                        DoMethod(iconlist,MUIM_IconList_Update);
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
                
            case WBHM_TYPE_OPEN:
                D(bug("Wanderer: WBHM_TYPE_OPEN\n"));
            
                {
                    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                    Object *child;
                    CONST_STRPTR buf = wbhm->wbhm_Data.Open.Name;
                    
                    while ((child = NextObject(&cstate)))
                    {
                        if (XGET(child, MUIA_UserData))
                        {
                            char *child_drawer = (char*)XGET(child, MUIA_IconWindow_Drawer);
                            if (child_drawer && !Stricmp(buf,child_drawer))
                            {
                                int is_open = XGET(child, MUIA_Window_Open);
                                if (!is_open)
                                    DoMethod(child, MUIM_IconWindow_Open);
                                else
                                {
                                    DoMethod(child, MUIM_Window_ToFront);
                                    set(child, MUIA_Window_Activate, TRUE);
                                }
                                return 0; 
                            }
                        }
                    }
        
                    {
                        /* Check if the window for this drawer is already opened */
                        Object *menustrip;
        
                        /* Create a new icon drawer window with the correct drawer being set */
                        Object *drawerwnd = IconWindowObject,
                            MUIA_UserData, 1,
                            MUIA_Window_Menustrip, (IPTR) menustrip = MUI_MakeObject(MUIO_MenustripNM, (IPTR) nm, (IPTR) NULL),
                            MUIA_IconWindow_IsRoot, FALSE,
                            MUIA_IconWindow_ActionHook, (IPTR) &hook_action,
                            MUIA_IconWindow_Drawer, (IPTR) buf,
                        End;
        
                        if (drawerwnd)
                        {
                            /* Get the drawer path back so we can use it also outside this function */
                            char *drw = (char*)XGET(drawerwnd,MUIA_IconWindow_Drawer);
        
                            /* We simply close the window here in case somebody like to to this...
                             * the memory is not freed until wb is closed however */
                            DoMethod
                            (
                                drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
                                (IPTR) drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE
                            );
        
                            /* If "Execute Command" entry is clicked open the execute window */
                            DoAllMenuNotifies(menustrip,drw);
        
                            /* Add the window to the application */
                            DoMethod(self, OM_ADDMEMBER, (IPTR) drawerwnd);
        
                            /* And now open it */
                            DoMethod(drawerwnd, MUIM_IconWindow_Open);
                        }
                    }
                }
                break;
        } /* switch */
        
        ReplyMsg((struct Message *) wbhm);
    }
    
    return 0;
}


IPTR Wanderer__MUIM_Wanderer_HandleNotify
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_INST_DATA;
    struct Message *notifyMessage;
    
    D(bug("Wanderer: got prefs change notify!\n"));
    
    while ((notifyMessage = GetMsg(data->pnotify_port)) != NULL)
    {
        ReplyMsg(notifyMessage);
    }
    
    DoMethod(data->wd_Prefs, MUIM_WandererPrefs_Reload);
    
    return 0;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_7
(
    Wanderer, NULL, MUIC_Application, NULL,
    OM_NEW,                          struct opSet *,
    OM_DISPOSE,                      Msg,
    OM_GET,                          struct opGet *,
    MUIM_Application_Execute,        Msg,
    MUIM_Wanderer_HandleTimer,       Msg,
    MUIM_Wanderer_HandleCommand,     Msg,
    MUIM_Wanderer_HandleNotify,      Msg
);
