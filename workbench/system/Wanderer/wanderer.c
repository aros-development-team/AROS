/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG
#define DEBUG 0

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

#include <aros/debug.h>

#include <aros/detach.h>

#include "iconwindow.h"
#include "wandererprefs.h"
#include "wanderer.h"

VOID DoAllMenuNotifies(Object *strip, char *path);
Object *FindMenuitem(Object* strip, int id);

extern Object *app;
struct Hook hook_standard;
struct Hook hook_action;


STRPTR GetScreenTitle(VOID)
{
    STATIC TEXT title[256];
    
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
    {NM_ITEM,  "ResetWanderer" },*/
  {NM_END}
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
    BPTR lock = NULL;
    
    if (cdptr != NULL) lock = Lock(*cdptr, SHARED_LOCK);
    
    execute_open_with_command(lock, NULL);
    
    if (lock) UnLock(lock);
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
    Object *item = FindMenuitem(strip, MEN_WANDERER_BACKDROP);
    Object *window = (Object *) XGET(app, MUIA_Wanderer_WorkbenchWindow);
    
    if (item != NULL)
    {
    	SET(window, MUIA_Window_Open, FALSE);
	SET(window, MUIA_IconWindow_IsBackdrop, XGET(item, MUIA_Menuitem_Checked));
    	SET(window, MUIA_Window_Open, TRUE);
    }
}

void icon_open()
{
    Object *window = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    DoMethod(window, MUIM_IconWindow_DoubleClicked);
}

void icon_information()
{
    Object                *window   = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);   
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        
        if ((int)entry != MUIV_IconList_NextSelected_End)
        {
            BPTR lock   = Lock(entry->filename, ACCESS_READ);
            BPTR parent = ParentDir(lock);
            
            kprintf("*** selected: %s\n", entry->filename);
            
            WBInfo(parent, FilePart(entry->filename), NULL);
            
            UnLock(parent);
            UnLock(lock);
        }
        else
        {
            break;
        }
    } while (TRUE);
}

void icon_delete(void)
{
    Object                *window   = (Object *) XGET(app, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        
        if ((int)entry != MUIV_IconList_NextSelected_End) 
        {
            BPTR lock   = Lock(entry->filename, ACCESS_READ);
            BPTR parent = ParentDir(lock);
            UnLock(lock);
            
            kprintf("*** selected: %s\n", entry->filename);
            
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
        else
        {
            break;
        }
    } while (TRUE);
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

AROS_UFH3
(
    void, hook_func_action,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1)
)
{
    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
	static char buf[1024];
	struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
	
	DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
	if ((int)ent == MUIV_IconList_NextSelected_End) return;

	if (msg->isroot)
	{
	    strcpy(buf,ent->label);
	    strcat(buf,":");
	}
        else
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
		DoMethod(app, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
                // FIXME: error handling
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
    Object                      *wd_Prefs,
                                *wd_ActiveWindow,
                                *wd_WorkbenchWindow;

    struct MUI_InputHandlerNode  wd_TimerIHN;
    struct MsgPort              *wd_CommandPort;
    struct MUI_InputHandlerNode  wd_CommandIHN;
    struct MsgPort              *wd_NotifyPort;
    struct MUI_InputHandlerNode  wd_NotifyIHN;
    struct NotifyRequest         pnr;
    
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
    	
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_INST_DATA;
        
        /*-- Setup hooks structures ----------------------------------------*/
        hook_standard.h_Entry = (HOOKFUNC) hook_func_standard;
        hook_action.h_Entry   = (HOOKFUNC) hook_func_action;
        
        // ---
        if ((data->wd_CommandPort = CreateMsgPort()) == NULL)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
    
        if ((data->wd_NotifyPort = CreateMsgPort()) == NULL)
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
        }
    
        RegisterWorkbench(data->wd_CommandPort);
        
        /* Setup command port handler --------------------------------------*/ 
        data->wd_CommandIHN.ihn_Signals = 1UL << data->wd_CommandPort->mp_SigBit;
        data->wd_CommandIHN.ihn_Object  = self;
        data->wd_CommandIHN.ihn_Method  = MUIM_Wanderer_HandleCommand;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_CommandIHN
        );
        
        /* Setup timer handler ---------------------------------------------*/
        data->wd_TimerIHN.ihn_Flags  = MUIIHNF_TIMER;
        data->wd_TimerIHN.ihn_Millis = 3000;
        data->wd_TimerIHN.ihn_Object = self;
        data->wd_TimerIHN.ihn_Method = MUIM_Wanderer_HandleTimer;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_TimerIHN
        );
        
        /* Setup filesystem notification handler ---------------------------*/
        data->wd_NotifyIHN.ihn_Signals = 1UL << data->wd_NotifyPort->mp_SigBit;
        data->wd_NotifyIHN.ihn_Object  = self;
        data->wd_NotifyIHN.ihn_Method  = MUIM_Wanderer_HandleNotify;
        
        DoMethod
        (
            self, MUIM_Application_AddInputHandler, (IPTR) &data->wd_NotifyIHN
        );
        
        /* Setup notification on prefs file --------------------------------*/
        data->pnr.nr_Name                 = "ENV:SYS/Wanderer.prefs";
        data->pnr.nr_Flags                = NRF_SEND_MESSAGE;
        data->pnr.nr_stuff.nr_Msg.nr_Port = data->wd_NotifyPort;
        
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
    
    if (data->wd_CommandPort)
    {
	/*
            They only have been added if the creation of the msg port was
	    successful
        */
	DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_TimerIHN);
	DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_CommandIHN);
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_NotifyIHN);
	
        UnregisterWorkbench(data->wd_CommandPort);
	
        EndNotify(&data->pnr);
        
        DeleteMsgPort(data->wd_NotifyPort);
        data->wd_NotifyPort = NULL;
        
        DeleteMsgPort(data->wd_CommandPort);
	data->wd_CommandPort = NULL;
        
        DisposeObject(data->wd_Prefs);
	data->wd_Prefs = NULL;
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

IPTR Wanderer__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Wanderer_ActiveWindow:
                data->wd_ActiveWindow = (Object *) tag->ti_Data;
                bug("*** wanderer active window: %p\n", tag->ti_Data);
                break;
		
	    case MUIA_Application_Iconified:
	        /* Wanderer itself cannot be iconified, 
		   just hide, instead.  */
	        tag->ti_Tag  = MUIA_ShowMe;
		tag->ti_Data = !tag->ti_Data;
		break; 
        }
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}

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
            
        case MUIA_Wanderer_ActiveWindow:
            *store = (IPTR) data->wd_ActiveWindow;
            break;
        
        case MUIA_Wanderer_WorkbenchWindow:
            *store = (IPTR) data->wd_WorkbenchWindow;
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
    
    data->wd_WorkbenchWindow = (Object *) DoMethod
    (
        self, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
    );
    
    if (data->wd_WorkbenchWindow != NULL)
    {
        DoMethod
        (
            data->wd_WorkbenchWindow, MUIM_KillNotify, MUIA_Window_CloseRequest
        );
        
        DoMethod
        (
            data->wd_WorkbenchWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR) self, 3, MUIM_CallHook, (IPTR) &hook_standard, (IPTR) wanderer_quit
        );

        Detach();
        
	DoSuperMethodA(CLASS, self, message);
        
        return RETURN_OK;
    }
    
    // FIXME: report error...
    
    return RETURN_ERROR;
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
    
    while ((wbhm = WBHM(GetMsg(data->wd_CommandPort))) != NULL)
    {
        D(bug("Wanderer: Recieved message from handler, type = %ld\n", wbhm->wbhm_Type));
        
        switch (wbhm->wbhm_Type)
        {
            case WBHM_TYPE_SHOW:
                D(bug("Wanderer: WBHM_TYPE_SHOW\n"));
                set(self, MUIA_ShowMe, TRUE);
                break;
            
            case WBHM_TYPE_HIDE:
                D(bug("Wanderer: WBHM_TYPE_HIDE\n"));
                set(self, MUIA_ShowMe, FALSE);
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
                    
                    DoMethod
                    (
                        app, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf
                    );
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
    
    while ((notifyMessage = GetMsg(data->wd_NotifyPort)) != NULL)
    {
        ReplyMsg(notifyMessage);
    }
    
    DoMethod(data->wd_Prefs, MUIM_WandererPrefs_Reload);
    
    return 0;
}

Object *Wanderer__MUIM_Wanderer_CreateDrawerWindow
(
    Class *CLASS, Object *self, 
    struct MUIP_Wanderer_CreateDrawerWindow *message
)
{
    Object *menustrip, *window = NULL;
    BOOL    isWorkbenchWindow = message->drawer == NULL ? TRUE : FALSE;
    
    /* Create a new icon drawer window with the correct drawer being set */
    window = IconWindowObject,
        MUIA_UserData,                     1,
        MUIA_Window_ScreenTitle,    (IPTR) GetScreenTitle(),
        MUIA_Window_Menustrip,      (IPTR) menustrip = MUI_MakeObject(MUIO_MenustripNM, (IPTR) nm, (IPTR) NULL),
        MUIA_IconWindow_ActionHook, (IPTR) &hook_action,
        
        MUIA_IconWindow_IsRoot,            isWorkbenchWindow ? TRUE : FALSE,
        MUIA_IconWindow_IsBackdrop,        isWorkbenchWindow ? TRUE : FALSE,
        isWorkbenchWindow ? 
            TAG_IGNORE    : 
            MUIA_IconWindow_Drawer, (IPTR) message->drawer,
    End;
    
    if (window != NULL)
    {
        /* Get the drawer path back so we can use it also outside this function */
        STRPTR drw;
        
        if (!isWorkbenchWindow) drw = (STRPTR) XGET(window, MUIA_IconWindow_Drawer);
        else                    drw = "RAM:";
        
        /* FIXME: should remove + dispose the window (memleak!) */
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
            (IPTR) window, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );
        
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_Activate, TRUE,
            (IPTR) _app(self), 3, MUIM_Set, MUIA_Wanderer_ActiveWindow, (IPTR) window
        );
        
        /* If "Execute Command" entry is clicked open the execute window */
        DoAllMenuNotifies(menustrip, drw);
        
        /* Add the window to the application */
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) window);
        
        /* And now open it */
        DoMethod(window, MUIM_IconWindow_Open);
    }
    
    return window;
}

/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_9
(
    Wanderer, NULL, MUIC_Application, NULL,
    OM_NEW,                           struct opSet *,
    OM_DISPOSE,                       Msg,
    OM_SET,                           struct opSet *,
    OM_GET,                           struct opGet *,
    MUIM_Application_Execute,         Msg,
    MUIM_Wanderer_HandleTimer,        Msg,
    MUIM_Wanderer_HandleCommand,      Msg,
    MUIM_Wanderer_HandleNotify,       Msg,
    MUIM_Wanderer_CreateDrawerWindow, struct MUIP_Wanderer_CreateDrawerWindow *
);
