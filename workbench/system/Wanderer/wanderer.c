/*
    Copyright © 2002-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#define INTUITION_NO_INLINE_STDARG
#define DEBUG 1

#include <exec/types.h>
#include <exec/memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <clib/alib_protos.h>
#include <dos/dostags.h>
#include <dos/notify.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <libraries/asl.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <string.h>

#include <libraries/mui.h>
#include <proto/workbench.h>
#include <aros/debug.h>
#include <workbench/handler.h>

#include "iconwindow.h"

extern struct Hook hook_action;

VOID DoAllMenuNotifies(Object *strip, char *path);
VOID LoadPrefs(VOID);

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
    {NM_ITEM,  "Backdrop",           "B", CHECKIT|MENUTOGGLE|CHECKED, NULL, (void*)MEN_WANDERER_BACKDROP},
    {NM_ITEM,  "Execute Command...", "E", NULL,               NULL, (void*)MEN_WANDERER_EXECUTE},
    /*{NM_ITEM,  "Redraw All" },
    {NM_ITEM,  "Update All" },
    {NM_ITEM,  "Last Message" },*/
    {NM_ITEM,  "Shell",              "W", NULL,               NULL, (void*)MEN_WANDERER_SHELL},
    {NM_ITEM,  "GUI Settings...",        NULL, NULL,          NULL, (void*)MEN_WANDERER_GUISETTINGS},
    {NM_ITEM,  "About...",           "?", NULL,               NULL, (void*)MEN_WANDERER_ABOUT},
    {NM_ITEM,  "Quit...",            "Q", NULL,               NULL, (void*)MEN_WANDERER_QUIT},

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
    {NM_ITEM,  "Open", "O", NULL, NULL, (void*) MEN_ICON_OPEN},
/*    {NM_ITEM,  "Close","C" },
    {NM_ITEM,  "Rename...", "R"},*/
    {NM_ITEM,  "Information...", "I", NULL, NULL, (void*) MEN_ICON_INFORMATION},
/*    {NM_ITEM,  "Snapshot", "S" },
    {NM_ITEM,  "Unsnapshot", "U" },
    {NM_ITEM,  "Leave Out", "L" },
    {NM_ITEM,  "Put Away", "P" },*/
    {NM_ITEM, NM_BARLABEL},
    {NM_ITEM,  "Delete...", NULL, NULL, NULL, (void*) MEN_ICON_DELETE},
/*    {NM_ITEM,  "Format Disk..." },
    {NM_ITEM,  "Empty Trash..." },

  {NM_TITLE, "Tools",          NULL, NM_MENUDISABLED},
    {NM_ITEM,  "ResetWanderer" },
  {NM_END}*/

};

/**************************************************************************
 This is the standard_hook for easy MUIM_CallHook callbacks
 It is initialized at the very beginning of the main program
**************************************************************************/
static struct Hook hook_standard;


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
	DoMethod(entry, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, entry, 4, MUIM_CallHook, &hook_standard, function, arg);
    }
}


/* Our global variables */

Object *app;
Object *root_iconwnd;
Object *root_menustrip;

CONST_STRPTR rootBG;
CONST_STRPTR dirsBG;

char *GetScreenTitle(void)
{
    static char title[256];
    /* AROS probably don't have graphics mem but without it look so empty */
    sprintf(title,"Wanderer  %ld graphics mem  %ld other mem",AvailMem(MEMF_CHIP),AvailMem(MEMF_FAST));
    return title;
}
/**************************************************************************
 This is a custom class inheriting from the Application Class.
 We need it support the (periodically) updating of the windows titles and
 we don't want to open timer.device by hand.
**************************************************************************/
struct Wanderer_Data
{
    struct MUI_InputHandlerNode timer_ihn;
    struct MUI_InputHandlerNode notify_ihn;
    struct MsgPort *notify_port;
    
    struct NotifyRequest         pnr;
    struct MsgPort              *pnotify_port;
    struct MUI_InputHandlerNode  pnotify_ihn;
};

#define MUIM_Wanderer_HandleTimer       0x8719129
#define MUIM_Wanderer_HandleNotify      0x871912a
#define MUIM_Wanderer_HandlePrefsNotify 0x871912b

STATIC IPTR Wanderer_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Wanderer_Data *data;

    obj = (Object*)DoSuperMethodA(cl,obj,(Msg)msg);
    if (!obj) return NULL;

    data = (struct Wanderer_Data*)INST_DATA(cl,obj);

    if (!(data->notify_port = CreateMsgPort()))
    {
	CoerceMethod(cl,obj,OM_DISPOSE);
	return NULL;
    }
    D(bug("Wanderer: notify port: %p\n", data->notify_port));

    if ((data->pnotify_port = CreateMsgPort()) == NULL)
    {
        CoerceMethod(cl,obj,OM_DISPOSE);
        return NULL;
    }
    
#ifdef __AROS__
    RegisterWorkbench(data->notify_port);
#endif

    /* Setup three input handlers */

    /* The first one is invoked every time we get a message from the Notify Port */
    data->notify_ihn.ihn_Signals = 1UL<<data->notify_port->mp_SigBit;
    bug("Wanderer: notify signal %ld\n", data->notify_ihn.ihn_Signals);
    data->notify_ihn.ihn_Object = obj;
    data->notify_ihn.ihn_Method = MUIM_Wanderer_HandleNotify;
    DoMethod(obj, MUIM_Application_AddInputHandler, &data->notify_ihn);


    /* The second one is a timer handler */
    data->timer_ihn.ihn_Flags = MUIIHNF_TIMER;
    /* called every second (this is only for timer input handlers) */
    data->timer_ihn.ihn_Millis = 3000;
    /* The following method of the given should be called if the
     * event happens */
    data->timer_ihn.ihn_Object = obj;
    data->timer_ihn.ihn_Method = MUIM_Wanderer_HandleTimer;

    DoMethod(obj, MUIM_Application_AddInputHandler, &data->timer_ihn);

    /* third one for prefs change notifies */
    data->pnotify_ihn.ihn_Signals = 1UL<<data->pnotify_port->mp_SigBit;
    bug("Wanderer: pnotify signal %ld\n", data->pnotify_ihn.ihn_Signals);
    data->pnotify_ihn.ihn_Object = obj;
    data->pnotify_ihn.ihn_Method = MUIM_Wanderer_HandlePrefsNotify;
    DoMethod(obj, MUIM_Application_AddInputHandler, &data->pnotify_ihn);

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
    
    return (IPTR)obj;
}

STATIC IPTR Wanderer_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct Wanderer_Data *data = (struct Wanderer_Data*)INST_DATA(cl,obj);
    if (data->notify_port)
    {
	/* They only have been added if the creation of the msg port was
	 * successful */
	DoMethod(obj, MUIM_Application_RemInputHandler, &data->timer_ihn);
	DoMethod(obj, MUIM_Application_RemInputHandler, &data->notify_ihn);
        DoMethod(obj, MUIM_Application_RemInputHandler, &data->pnotify_ihn);
#ifdef __AROS__
	UnregisterWorkbench(data->notify_port);
#endif
	EndNotify(&data->pnr);
        DeleteMsgPort(data->pnotify_port);
        DeleteMsgPort(data->notify_port);
	data->notify_port = NULL;
    }
    return (IPTR)DoSuperMethodA(cl,obj,(Msg)msg);
}

STATIC IPTR Wanderer_HandleTimer(struct IClass *cl, Object *obj, Msg msg)
{
    Object *cstate = (Object*)(((struct List*)XGET(obj, MUIA_Application_WindowList))->lh_Head);
    Object *child;
    char *scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	set(child, MUIA_Window_ScreenTitle, scr_title);
    return 0; /* irrelevant */
}

STATIC IPTR Wanderer_HandleNotify(struct IClass *cl, Object *obj, Msg msg)
{
    struct Wanderer_Data    *data = (struct Wanderer_Data*) INST_DATA(cl,obj);
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
                        Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
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
                    Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
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
                            MUIA_Window_Menustrip, menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
                            MUIA_IconWindow_IsRoot, FALSE,
                            MUIA_IconWindow_ActionHook, &hook_action,
                            MUIA_IconWindow_Drawer, buf,
                        End;
        
                        if (drawerwnd)
                        {
                            /* Get the drawer path back so we can use it also outside this function */
                            char *drw = (char*)XGET(drawerwnd,MUIA_IconWindow_Drawer);
        
                            /* We simply close the window here in case somebody like to to this...
                             * the memory is not freed until wb is closed however */
                            DoMethod(drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);
        
                            /* If "Execute Command" entry is clicked open the execute window */
                            DoAllMenuNotifies(menustrip,drw);
        
                            /* Add the window to the application */
                            DoMethod(app,OM_ADDMEMBER,drawerwnd);
        
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


STATIC IPTR Wanderer_HandlePrefsNotify(struct IClass *cl, Object *obj, Msg msg)
{
    struct Wanderer_Data *data = (struct Wanderer_Data*) INST_DATA(cl,obj);
    struct Message *notifyMessage;
    
    D(bug("Wanderer: got prefs change notify!\n"));
    
    while ((notifyMessage = GetMsg(data->pnotify_port)) != NULL)
    {
        ReplyMsg(notifyMessage);
    }
    
    LoadPrefs();
    
    return 0;
}

/* Use this macro for dispatchers if you don't want #ifdefs */
BOOPSI_DISPATCHER(IPTR,Wanderer_Dispatcher,cl,obj,msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Wanderer_New(cl,obj,(APTR)msg);
	case OM_DISPOSE: return Wanderer_Dispose(cl,obj,(APTR)msg);
	case MUIM_Wanderer_HandleTimer: return Wanderer_HandleTimer(cl,obj,(APTR)msg);
	case MUIM_Wanderer_HandleNotify: return Wanderer_HandleNotify(cl,obj,(APTR)msg);
        case MUIM_Wanderer_HandlePrefsNotify: return Wanderer_HandlePrefsNotify(cl,obj,(APTR)msg);
    }
    return DoSuperMethodA(cl,obj,msg);
}

struct MUI_CustomClass *CL_Wanderer;

#define WandererObject BOOPSIOBJMACRO_START(CL_Wanderer->mcc_Class)


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
                    DoMethod(iconList, MUIM_IconList_NextSelected, &entry);
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
                    DoMethod(iconList, MUIM_IconList_NextSelected, &entry);
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

/**************************************************************************
 Start all menu notifies
**************************************************************************/
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
	DoMethod(item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, app, 7, MUIM_Application_PushMethod, app, 4, MUIM_CallHook, &hook_standard, wanderer_backdrop, strip);
    }
}


#ifdef __AROS__
LONG            __detacher_must_wait_for_signal = SIGBREAKF_CTRL_F;
struct Process *__detacher_process              = NULL;
STRPTR          __detached_name                 = "Wanderer";

void DoDetach(void)
{
    kprintf("LoadWB.DoDetach\n");

    /* If there's a detacher, tell it to go away */
    if (__detacher_process)
    {
	Signal((struct Task *)__detacher_process, __detacher_must_wait_for_signal);
    }
}
#endif

BOOL ReadLine(BPTR fh, STRPTR buffer, ULONG size)
{
    if (FGets(fh, buffer, size) != NULL)
    {
        ULONG last = strlen(buffer) - 1;
        if (buffer[last] == '\n') buffer[last] = '\0';
        
        return TRUE;
    }
    
    return FALSE;
}

VOID LoadPrefs(VOID)
{
    BPTR fh;
    
    if ((fh = Open("ENV:SYS/Wanderer.prefs", MODE_OLDFILE)) != NULL)
    {
        STRPTR buffer = NULL;
        LONG   size;
        
        Seek(fh, 0, OFFSET_END);
        size = Seek(fh, 0, OFFSET_BEGINNING) + 2;
        
        if ((buffer = AllocVec(size, MEMF_ANY)) != NULL)
        {
            if (!ReadLine(fh, buffer, size)) goto end;
            if (rootBG == NULL)
            {
                rootBG = StrDup(buffer);
            }
            else if (strcmp(rootBG, buffer) != 0)
            {
                FreeVec(rootBG);
                rootBG = StrDup(buffer);
                
                if (rootBG != NULL)
                {
                    SET
                    (
                        (Object *) XGET(root_iconwnd, MUIA_IconWindow_IconList),
                        MUIA_Background, (IPTR) rootBG
                    );
                }
            }
            
            if (!ReadLine(fh, buffer, size)) goto end;
            if (dirsBG == NULL)
            {
                dirsBG = StrDup(buffer);
            }
            else if (strcmp(dirsBG, buffer) != 0)
            {
                FreeVec(dirsBG);
                dirsBG = StrDup(buffer);
                
                if (dirsBG != NULL)
                {
                    Object *cstate = (Object*)(((struct List*)XGET(_app(root_iconwnd), MUIA_Application_WindowList))->lh_Head);
                    Object *child;
        
                    while ((child = NextObject(&cstate)))
                    {
                        if (child != root_iconwnd && XGET(child, MUIA_UserData))
                        {
                            SET
                            (
                                (Object *) XGET(child, MUIA_IconWindow_IconList),
                                MUIA_Background, (IPTR) dirsBG
                            );
                        }
                    }
                }
            }
            
end:        FreeVec(buffer);
        }
        
        Close(fh);
    }
}

VOID FreePrefs()
{
    if (rootBG != NULL) FreeVec(rootBG);
    if (dirsBG != NULL) FreeVec(dirsBG);
}
struct Hook hook_action;

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

	DoMethod(msg->iconlist, MUIM_IconList_NextSelected, &ent);
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
	    Object *cstate = (Object*)(((struct List*)XGET(app, MUIA_Application_WindowList))->lh_Head);
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
		    MUIA_Window_Menustrip, menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
		    MUIA_IconWindow_IsRoot, FALSE,
		    MUIA_IconWindow_ActionHook, &hook_action,
		    MUIA_IconWindow_Drawer, buf,
		End;

		if (drawerwnd)
		{
	    	    /* Get the drawer path back so we can use it also outside this function */
		    char *drw = (char*)XGET(drawerwnd,MUIA_IconWindow_Drawer);

		    /* We simply close the window here in case somebody like to to this...
		     * the memory is not freed until wb is closed however */
		    DoMethod(drawerwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, drawerwnd, 3, MUIM_Set, MUIA_Window_Open, FALSE);

		    /* If "Execute Command" entry is clicked open the execute window */
		    DoAllMenuNotifies(menustrip,drw);

		    /* Add the window to the application */
		    DoMethod(app,OM_ADDMEMBER,drawerwnd);

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
    } else
    if (msg->type == ICONWINDOW_ACTION_CLICK)
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
    } else
    if (msg->type == ICONWINDOW_ACTION_ICONDROP)
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
		    DoMethod(iconlist, MUIM_IconList_NextSelected, &ent);
		    if ((int)ent == MUIV_IconList_NextSelected_End) break;

		    Printf("%s\n",ent->filename);
		} while(1);
	    }
	}
    }
}

/**************************************************************************
 Our main entry
**************************************************************************/
int main(void)
{
    hook_standard.h_Entry = (HOOKFUNC)hook_func_standard;
    hook_action.h_Entry = (HOOKFUNC)hook_func_action;

    /* Create the Wanderer class which inherits from MUIC_Application (ApplicationClass) */
    CL_Wanderer = MUI_CreateCustomClass(NULL,MUIC_Application,NULL,sizeof(struct Wanderer_Data), Wanderer_Dispatcher);
    if (!CL_Wanderer)
    {
	return 20;
    }

    LoadPrefs();

    app = WandererObject,
	MUIA_Application_Title, (IPTR) "Wanderer",
	MUIA_Application_Base, (IPTR) "WANDERER",
	MUIA_Application_Version, (IPTR) "$VER: Wanderer 0.1 (10.12.02)",
	MUIA_Application_Description, (IPTR) "The AROS filesystem GUI",
	MUIA_Application_SingleTask, TRUE,
    	SubWindow, root_iconwnd = IconWindowObject,
	    MUIA_UserData, 1,
	    MUIA_Window_Menustrip, root_menustrip = MUI_MakeObject(MUIO_MenustripNM,nm,NULL),
	    MUIA_Window_ScreenTitle, GetScreenTitle(),
            MUIA_IconWindow_IsRoot, TRUE,
	    MUIA_IconWindow_IsBackdrop, TRUE,
	    MUIA_IconWindow_ActionHook, &hook_action,
	    End,
	End;

    if (app)
    {
	ULONG sigs = 0;

	DoMethod(root_iconwnd, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, app, 3, MUIM_CallHook, &hook_standard, wanderer_quit);

	/* If "Execute Command" entry is clicked open the execute window */
	DoAllMenuNotifies(root_menustrip,"RAM:");

	/* And now open it */
	DoMethod(root_iconwnd, MUIM_IconWindow_Open);

	DoDetach();

	while((LONG) DoMethod(app, MUIM_Application_NewInput, &sigs) != MUIV_Application_ReturnID_Quit)
	{
	    if (sigs)
	    {
		sigs = Wait(sigs | SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D);
		if (sigs & SIGBREAKF_CTRL_C) break;
		if (sigs & SIGBREAKF_CTRL_D) break;
	    }
	}

	MUI_DisposeObject(app);
    }

    FreePrefs();
    
    MUI_DeleteCustomClass(CL_Wanderer);
    return 0;
}
