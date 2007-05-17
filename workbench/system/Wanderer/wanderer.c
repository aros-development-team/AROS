/*
    Copyright  2004-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#define WANDERER_DEFAULT_BACKDROP
//#define WANDERER_DEFAULT_SHOWALL
//#define WANDERER_DEFAULT_SHOWHIDDEN

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>
#include <zune/customclasses.h>
#include <dos/notify.h>
#include <workbench/handler.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/dos.h>
#include <proto/workbench.h>
#include <proto/layers.h>
#include <proto/alib.h>

#include <string.h>
#include <stdio.h>
#include <time.h>

#include <aros/detach.h>
#include <prefs/wanderer.h>

#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindowcontents.h"
#include "wandererprefs.h"
#include "wandererprefsintern.h"
#include "filesystems.h"
#include "wanderer.h"
#include "../../libs/muimaster/classes/iconlist.h"
#include "../../libs/muimaster/classes/iconlist_attributes.h"
#include "locale.h"

#define VERSION "$VER: Wanderer 0.65 (16.04.2007) The AROS Dev Team"

#define KeyButton(name,key) TextObject, ButtonFrame, MUIA_Font, MUIV_Font_Button, MUIA_Text_Contents, (IPTR)(name), MUIA_Text_PreParse, "\33c", MUIA_Text_HiChar, (IPTR)(key), MUIA_ControlChar, key, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Background, MUII_ButtonBack, End

extern IPTR InitWandererPrefs(void);
VOID        DoAllMenuNotifies(Object *strip, STRPTR path);
Object      *FindMenuitem(Object* strip, int id);
Object      *__CreateWandererIntuitionMenu__(BOOL isRoot, BOOL useBackdrop);
void        wanderer_menufunc_window_update(void);
void        execute_open_with_command(BPTR cd, STRPTR contents);
void        DisposeCopyDisplay(struct MUIDisplayObjects *d);
BOOL        CreateCopyDisplay(UWORD flags, struct MUIDisplayObjects *d);

/* Stored in the main wanderer executable */
extern Object        *_WandererIntern_AppObj;
extern Class 		 *_WandererIntern_CLASS;
/* Internal Hooks */
struct Hook          _WandererIntern_hook_standard;
struct Hook          _WandererIntern_hook_action;
struct Hook          _WandererIntern_hook_backdrop;

static unsigned char strtochar(STRPTR st)
{
    return *st++;
}

/*** Instance Data **********************************************************/
struct Wanderer_DATA
{
	struct Screen 				*wd_Screen;

    Object                      *wd_Prefs,
                                *wd_ActiveWindow,
                                *wd_WorkbenchWindow;

    struct MUI_InputHandlerNode  wd_TimerIHN;
    struct MsgPort              *wd_CommandPort;
    struct MUI_InputHandlerNode  wd_CommandIHN;
    struct MsgPort              *wd_NotifyPort;
    struct MUI_InputHandlerNode  wd_NotifyIHN;
    struct NotifyRequest         pnr;

    IPTR                         wd_PrefsIntern;
	BOOL                         wd_Option_BackDropMode;
};

/*** Macros *****************************************************************/
#define SETUP_WANDERER_INST_DATA struct Wanderer_DATA *data = INST_DATA(CLASS, self)

/**************************************************************************
* HOOK FUNCS                                                              *
**************************************************************************/

AROS_UFH3
(
    BOOL, Wanderer__HookFunc_DisplayCopyFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct dCopyStruct *, obj, A2),
    AROS_UFHA(APTR, unused_param, A1)
)
{
    AROS_USERFUNC_INIT
    
    char     *c = NULL;
    unsigned int difftime;

    struct MUIDisplayObjects *d = (struct MUIDisplayObjects *) obj->userdata;

    
    if ((obj->flags & ACTION_UPDATE) == 0)
    {
        d->updateme = TRUE;

        if ((obj->filelen < 8192) && (d->numfiles > 0))
        {
            d->smallobjects++;
            if (d->smallobjects >= 20) d->smallobjects = 0;
        }
        else
        {
            d->smallobjects = 0;
        }
        if (d->smallobjects > 0) d->updateme = FALSE;
        if (d->updateme)
        {
            SET(d->fileObject, MUIA_Text_Contents, obj->file);
            SET(d->sourceObject, MUIA_Text_Contents, obj->spath);
        }
    }
    if (d->action != ACTION_DELETE) 
    {

        d->bytes += obj->actlen;
        if ((obj->flags & ACTION_UPDATE) == 0)
        {
            if (d->updateme)
            {
                SET(d->gauge, MUIA_Gauge_Current, 0);
                SET(d->destObject, MUIA_Text_Contents, obj->dpath);
            }
            d->numfiles++;
        }
        else
        {
            if (d->updateme &&(obj->totallen <= obj->filelen))
            {
                double rate = (double) (((double) obj->totallen) / (((double) obj->difftime) / ((double) CLOCKS_PER_SEC))) / 1024.0;
                if (rate < 1024.0) sprintf(d->SpeedBuffer, "Speed: %.2f kBytes/s",  rate); else sprintf(d->SpeedBuffer, "Speed: %.2f MBytes/s",  rate / 1024.0);
                SetAttrs(d->gauge, MUIA_Gauge_Current, (ULONG) (32768.0 * (double) obj->totallen / (double) obj->filelen),  MUIA_Gauge_InfoText, d->SpeedBuffer, TAG_DONE);
            }
        }


        if (d->updateme)
        {
            if (d->bytes < 1048576)
            {
                if (obj->filelen < 1048576)
                {
                    sprintf(
                        d->Buffer, "# of files: %ld   Actual: %.2f kBytes   Total: %.2f kBytes", 
                        d->numfiles, (double) obj->filelen / 1024.0, (double) d->bytes / 1024.0
                    );
                }
                else
                {
                    sprintf(
                        d->Buffer, "# of files: %ld   Actual: %.2f MBytes   Total: %.2f kBytes", 
                        d->numfiles, (double) obj->filelen / 1048576.0, (double) d->bytes / 1024.0
                    );
                }
            }
            else
            {
                if (obj->filelen < 1048576)
                {
                    sprintf(
                        d->Buffer, "# of files: %ld   Actual: %.2f kBytes   Total: %.2f MBytes", 
                        d->numfiles, (double) obj->filelen / 1024.0, (double) d->bytes / 1048576.0
                    );
                }
                else
                {
                    sprintf(
                        d->Buffer, "# of files: %ld   Actual: %.2f MBytes   Total: %.2f MBytes", 
                        d->numfiles, (double) obj->filelen / 1048576.0, (double) d->bytes / 1048576.0
                    );
                }
            }
            SET(d->performanceObject, MUIA_Text_Contents, d->Buffer);
        }
    }
    
    DoMethod(d->copyApp, MUIM_Application_InputBuffered);

    /* read the stopflag and return TRUE if the user wanted to stop actionDir() */

    if (d->stopflag == 1) return TRUE; else return FALSE;

    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    ULONG, Wanderer__HookFunc_AskDeleteFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct dCopyStruct *, obj, A2),
    AROS_UFHA(APTR, unused_param, A1)
)
{
    AROS_USERFUNC_INIT
    
    ULONG back = DELMODE_NONE;

    UWORD    ret = 0;
    char     *string = NULL;
    
    if (obj->file) 
    {
        if (obj->type == 0) 
        {
            string = CombineString("Really delete file\n\033b%s\033n\nlocated in\n\033b%s\033n ?", 
                obj->file, obj->spath);
        }
        else if (obj->type == 1) 
        {
            string = CombineString("Do you wish to unprotect file\n\033b%s\033n\nlocated in\n\033b%s\033n ?", 
                obj->file, obj->spath);
        }
        else if (obj->type == 2) 
        {
            string = CombineString("Really overwrite file\n\033b%s\033n\nlocated in\n\033b%s\033n ?", 
                obj->file, obj->spath);
        }
        else 
        {
            string = CombineString("Can't access file\n\033b%s\033n\nlocated in\n\033b%s\033n ?", 
                obj->file, obj->spath);
        }
    } 
    else 
    {
        if (obj->type == 0) string = CombineString("Really delete drawer\n\033b%s\033n ?", obj->spath);
        else if (obj->type == 1) string = CombineString("Do you wish to unprotect drawer\n\033b%s\033n ?", obj->spath);
        else if (obj->type == 3) string = CombineString("Can't access drawer\n\033b%s", obj->spath);
    }

    if (string) 
    {
        if (obj->type == 0) ret = AskChoiceCentered("Delete Requester:", string, "_Yes|Yes to _All|_No|No _to ALL", 0);
        else if (obj->type == 1) ret = AskChoiceCentered("Protection Requester:", string, "_Unprotect|Unprotect _All|_No|No _to ALL", 0);
        else if (obj->type == 2) ret = AskChoiceCentered("Overwrite Requester:", string, "_Overwrite|Overwrite _All|_No|No _to ALL", 0);
        else ret = AskChoiceCentered("Overwrite Requester:", string, "_Skip|_Abort", 0);
        freeString(NULL, string);
    }

    if (ret == 0) back = DELMODE_NONE;
    else if (ret == 1) back = DELMODE_DELETE;
    else if (ret == 2) back = DELMODE_ALL;
    else if (ret == 3) back = DELMODE_NO;

    return back;
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, Wanderer__HookFunc_ActionFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1)
)
{
    AROS_USERFUNC_INIT

    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
        static unsigned char  buf[1024];

        struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;

        DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
        if ((int)ent == MUIV_IconList_NextSelected_End) return;

		IPTR                  offset = strlen(ent->filename) - 5;

        if ((msg->isroot) && (!Stricmp(ent->filename + offset, ":Disk")))
		{
			strcpy(buf,ent->label);
			strcat(buf,":");
		}
        else
        {
            strcpy(buf,ent->filename);
        }

D(bug("[WANDERER] Wanderer__HookFunc_ActionFunc: ICONWINDOW_ACTION_OPEN - offset = %d, buf = %s\n", offset, buf);)
    
        if  ( (ent->type == ST_ROOT) || (ent->type == ST_USERDIR) )
        {
            Object *cstate = (Object*)(((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
            Object *prefs = (Object*) XGET(_app(obj), MUIA_Wanderer_Prefs);
            Object *child;

            /* open new window if root or classic navigation set */
            if ( (msg->isroot) || (XGET(prefs, MUIA_IconWindowExt_Toolbar_NavigationMethod) == WPD_NAVIGATION_CLASSIC) )
            {
                while ((child = NextObject(&cstate)))
                {
                    if (XGET(child, MUIA_UserData))
                    {
                        STRPTR child_drawer = (STRPTR)XGET(child, MUIA_IconWindow_Location);
                        if (child_drawer && !Stricmp(buf,child_drawer))
                        {
                            BOOL is_open = ( BOOL )XGET(child, MUIA_Window_Open);
                            
                            if (!is_open)
                            {
                                DoMethod(child, MUIM_IconWindow_Open);
                            }
                            else
                            {
                                DoMethod(child, MUIM_Window_ToFront);
                                SET(child, MUIA_Window_Activate, TRUE);
                            }
                            
                            return;
                        }
                    }
                } 
        
                /* Check if the window for this drawer is already opened */
                DoMethod(_WandererIntern_AppObj, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
                // FIXME: error handling
            }
            else
            {
                /* open drawer in same window */
                SET(obj, MUIA_IconWindow_Location, (IPTR) buf);
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
    else  if (msg->type == ICONWINDOW_ACTION_DIRUP)
    {     
                    
        STRPTR actual_drawer = (STRPTR)XGET(obj, MUIA_IconWindow_Location);
        STRPTR parent_drawer = strrchr(actual_drawer,'/');
        STRPTR root_drawer = strrchr(actual_drawer,':');
        
        /* check if dir is not drive root dir */
        if ( strlen(root_drawer) > 1 )
        {
            /* check if second or third level directory*/
            if (!parent_drawer)
            {
                (*(root_drawer+1)) = 0;
                SET(obj, MUIA_IconWindow_Location, actual_drawer);
                
            }
            else
            {
                (*parent_drawer) = 0;
                SET(obj, MUIA_IconWindow_Location, actual_drawer);
            } 
            
        }
    } 
    else if (msg->type == ICONWINDOW_ACTION_CLICK)
    {
        if (!msg->click->shift)
        {
            Object *cstate = (Object*)(((struct List*)XGET(_WandererIntern_AppObj, MUIA_Application_WindowList))->lh_Head);
            Object *child;
    
            while ((child = NextObject(&cstate)))
            {
                if (XGET(child, MUIA_UserData))
                {
                    if (child != obj)  DoMethod(child, MUIM_IconWindow_UnselectAll);
                }
            }
        }
    } 
    else if (msg->type == ICONWINDOW_ACTION_ICONDROP)
    {
        IPTR destination_path;

        struct IconList_Drop *drop = (struct IconList_Drop *)msg->drop;

        if (drop)
        {
            /* get path of DESTINATION iconlist*/
            destination_path = drop->destination_string;

            if ( !destination_path ) return;
                
            /* get SOURCE entries */

            struct MUIDisplayObjects dobjects;
            struct Hook displayCopyHook;
            struct Hook displayDelHook;

            displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
            displayDelHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskDeleteFunc;
            

            if (CreateCopyDisplay(ACTION_COPY, &dobjects))
            {
                struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
    
                /* process all selected entries */
                do
                {
                    DoMethod(drop->source_iconlistobj, MUIM_IconList_NextSelected, (IPTR) &ent);

                    /* if not end of selection, process */
                    if ( (int)ent != MUIV_IconList_NextSelected_End )
                    {
D(bug("[WANDERER] drop entry: %s dropped in %s\n", ent->filename, destination_path));

                        /* copy via filesystems.c */
D(bug("[WANDERER] CopyContent \"%s\" to \"%s\"\n", ent->filename, destination_path ));
                        CopyContent(NULL, ent->filename, destination_path, TRUE, ACTION_COPY, &displayCopyHook, &displayDelHook, (APTR) &dobjects);
                    }
                } 
                while ( (int)ent != MUIV_IconList_NextSelected_End );
                DisposeCopyDisplay(&dobjects);
            }
            /* update list contents */
            DoMethod(drop->destination_iconlistobj,MUIM_IconList_Update);
        }
    }
    else if (msg->type == ICONWINDOW_ACTION_APPWINDOWDROP)
    {
        struct Screen *wscreen = NULL;
        struct Layer *layer;

        /* get wanderers screen struct and the layer located at cursor position afterwards */
        get( obj, MUIA_Window_Screen, &wscreen);
        layer = WhichLayer(&wscreen->LayerInfo,wscreen->MouseX,wscreen->MouseY);

        if (layer)
        {
            struct Window *win = (struct Window *) layer->Window;
            if (win)
            {
                struct List AppList;
                ULONG files = 0;
                BOOL  fail  = FALSE;
                NewList(&AppList);

                struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
                /* process all selected entries */
                do 
                {
                    DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR) &ent);
                    /*  if not end of selection, process */
                    if ( (int)ent != MUIV_IconList_NextSelected_End )
                    {
                        struct AppW *a = AllocVec(sizeof(struct AppW), MEMF_CLEAR);
                        if (a)
                        {
                            a->name = AllocVec(strlen(ent->filename)+1, MEMF_CLEAR);
                            if (a->name)
                            {
                                files++;
                                strcpy(a->name, ent->filename);
                                AddTail(&AppList, (struct Node *) a);
                            }
                            else
                            {
                                FreeVec(a);
                                fail = TRUE;
                            }
                        } 
                        else fail = TRUE;
                    }
                } 
                while ( ((int)ent != MUIV_IconList_NextSelected_End ) && !fail);
                
                if (!fail && (files > 0))
                {
                    STRPTR *filelist = AllocVec(sizeof(STRPTR) * files, MEMF_CLEAR);
                    if (filelist != NULL)
                    {
                        STRPTR *flist = filelist;
                        if (!IsListEmpty(&AppList))
                        {
                            struct Node *succ;
                            struct Node *s = AppList.lh_Head;
                            while (((succ = ((struct Node*) s)->ln_Succ) != NULL) && !fail)
                            {
                                *flist ++ = ((struct AppW *) s)->name;
                                s =  succ;
                            }

D(bug("[WANDERER] AppWindowMsg: win:%s files:%s mx:%d my:%d\n",win->Title, filelist, wscreen->MouseX - win->LeftEdge, wscreen->MouseY - win->TopEdge);)
                            /* send appwindow msg struct containing selected files to destination */
                            SendAppWindowMessage(win, files, filelist, 0, wscreen->MouseX - win->LeftEdge, wscreen->MouseY - win->TopEdge, 0, 0);

                        }
                        FreeVec(filelist);
                    }
                }
                if (!IsListEmpty(&AppList))
                {
                    struct Node *succ;
                    struct Node *s = AppList.lh_Head;
                    while (((succ = ((struct Node*) s)->ln_Succ) != NULL))
                    {
                        if ( ((struct AppW *) s)->name != NULL ) 
                            FreeVec(((struct AppW *) s)->name);
                        if ( s != NULL ) 
                            FreeVec(s);
                        s =  succ;
                    }
                }

            }
        }       
        

    }

    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, Wanderer__HookFunc_StandardFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
    AROS_USERFUNC_INIT
    
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);
    if (func) func((ULONG *)(funcptr + 1));
    
    AROS_USERFUNC_EXIT
}

AROS_UFH3
(
    void, Wanderer__HookFunc_BackdropFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
    AROS_USERFUNC_INIT

D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc()\n"));
	struct Wanderer_DATA *data = INST_DATA(_WandererIntern_CLASS, _WandererIntern_AppObj);

D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Private data @ %x\n", data));

	if (!data->wd_WorkbenchWindow)
	{
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: No Workbench Window\n"));
		return;
	}
	
	BOOL    wb_iscurrentlybd = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_IconWindow_IsBackdrop);

	if (wb_iscurrentlybd != data->wd_Option_BackDropMode)
	{
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Backdrop mode change requested!\n"));

		BOOL          isOpen = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_Window_Open);
		Object        *win_Active = NULL;

D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Disposing of existing Workbench window Obj ..\n"));
		if (isOpen)
			SET(data->wd_WorkbenchWindow, MUIA_Window_Open, FALSE);

		if (data->wd_WorkbenchWindow == data->wd_ActiveWindow)
		{
			data->wd_ActiveWindow = NULL;
		}
		else
		{
			win_Active = data->wd_ActiveWindow;
		}

		/* Kill our close request notification .. */
        DoMethod
        (
            data->wd_WorkbenchWindow, MUIM_KillNotify, MUIA_Window_CloseRequest
        );
		
		/* .. And dispose of the window */
		DoMethod(_WandererIntern_AppObj, OM_REMMEMBER, data->wd_WorkbenchWindow);
        MUI_DisposeObject(data->wd_WorkbenchWindow);
		data->wd_WorkbenchWindow = NULL;

#if defined(DEBUG)
		if (data->wd_Option_BackDropMode)
		{
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Creating new Workbench window Obj (BACKDROP MODE)..\n"));
		}
		else
		{
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Creating new Workbench window Obj (NORMAL MODE)..\n"));
		}
#endif

		data->wd_WorkbenchWindow = (Object *) DoMethod
		(
			_WandererIntern_AppObj, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
		);

		if ((data->wd_WorkbenchWindow) && (isOpen))
		{
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Making Workbench window visable..\n"));
			DoMethod(data->wd_WorkbenchWindow, MUIM_IconWindow_Open);
			DoMethod(data->wd_WorkbenchWindow, MUIM_Window_ToBack);
		}

		if (win_Active)
		{
			SET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow, win_Active);
		}
		else if (data->wd_WorkbenchWindow)
		{
			SET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow, data->wd_WorkbenchWindow);
		}
	}
    AROS_USERFUNC_EXIT
}

/******** code from workbench/c/Info.c *******************/
static void fmtlarge(UBYTE *buf, ULONG num)
{
    UQUAD d;
    UBYTE ch;
    struct
    {
        ULONG val;
        LONG  dec;
    } array =
    {
        num,
        0
    };

    if (num >= 1073741824)
    {
        array.val = num >> 30;
        d = ((UQUAD)num * 10 + 536870912) / 1073741824;
        array.dec = d % 10;
        //ch = 'G';
        ch = strtochar((STRPTR)_(MSG_MEM_G));
    }
    else if (num >= 1048576)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 524288) / 1048576;
        array.dec = d % 10;
        //ch = 'M';
        ch = strtochar((STRPTR)_(MSG_MEM_M));
    }
    else if (num >= 1024)
    {
        array.val = num >> 10;
        d = (num * 10 + 512) / 1024;
        array.dec = d % 10;
        //ch = 'K';
        ch = strtochar((STRPTR)_(MSG_MEM_K));
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        //ch = 'B';
    ch = strtochar((STRPTR)_(MSG_MEM_B));
    }

    if (!array.dec && (d > array.val * 10))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);
    while (*buf) { buf++; }
    *buf++ = ch;
    *buf   = '\0';
}

STRPTR GetScreenTitle(VOID)
{
    STATIC TEXT title[256];
    UBYTE chip[10], fast[10];
    fmtlarge(chip,AvailMem(MEMF_CHIP));
    fmtlarge(fast,AvailMem(MEMF_FAST));
  
    /* AROS probably don't have graphics mem but without it looks so empty */
    sprintf(title, _(MSG_SCREENTITLE), chip, fast);

    return title;
}

STRPTR GetUserScreenTitle(Object *self)
{
  
    char *screentitlestr;

    Object *prefs = NULL;

	
    GET(self, MUIA_IconWindowExt_ScreenTitle_String, &screentitlestr);
    D(bug("[WANDERER] GetUserScreenTitle: Wb screen title = %s\n", screentitlestr));
    	
    return screentitlestr;
}

/* Expand a passed in env: string to its full location */
/* Wanderer doesnt free this mem at the moment but should 
   incase it is every closed */
STRPTR ExpandEnvName(STRPTR env_path)
{
	BOOL     ok = FALSE;
	char     tmp_envbuff[1024];
	STRPTR   fullpath = NULL;
	BPTR     env_lock = NULL;

	env_lock = Lock("ENV:", SHARED_LOCK);
	if (env_lock)
	{
		if (NameFromLock(env_lock, tmp_envbuff, 256)) ok = TRUE;
		UnLock(env_lock);
	}
	
	if (ok)
	{
		if ((fullpath = AllocVec(strlen(tmp_envbuff) + strlen(env_path) + 1 + 1 - 4, MEMF_CLEAR | MEMF_PUBLIC)) != NULL)
		{
			strcpy(fullpath, tmp_envbuff);
			AddPart(fullpath, env_path + 4, 1019);
			return fullpath;
		}			
	}

	//We couldnt expand it so just use as is ..
	return env_path;
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
	
	MEN_WINDOW_NEW_DRAWER,
	MEN_WINDOW_OPEN_PARENT,
	MEN_WINDOW_CLOSE,
	MEN_WINDOW_UPDATE,

	MEN_WINDOW_SELECT,
	MEN_WINDOW_CLEAR,

	MEN_WINDOW_SNAP_WIN,
	MEN_WINDOW_SNAP_ALL,

	MEN_WINDOW_VIEW_ICON,
	MEN_WINDOW_VIEW_DETAIL,
	MEN_WINDOW_VIEW_ALL,

	MEN_WINDOW_SORT_NOW,
    MEN_WINDOW_SORT_NAME,
    MEN_WINDOW_SORT_TYPE,
    MEN_WINDOW_SORT_DATE,
    MEN_WINDOW_SORT_SIZE,
    MEN_WINDOW_SORT_REVERSE,
    MEN_WINDOW_SORT_TOPDRAWERS,
	MEN_WINDOW_SORT_GROUP,
    
    MEN_ICON_OPEN,
    MEN_ICON_RENAME,
    MEN_ICON_INFORMATION,
    MEN_ICON_DELETE,
};



/**************************************************************************
Open the execute window. Similar to below but you can also set the
command. Called when item is openend
**************************************************************************/
void execute_open_with_command(BPTR cd, STRPTR contents)
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
VOID wanderer_menufunc_wanderer_execute(STRPTR *cdptr)
{
    //TODO: remove the STRPTR *cdptr from top
    //TODO:remove this commented out stuff
    //BPTR lock = NULL;
    //if (cdptr != NULL) lock = Lock(*cdptr, SHARED_LOCK);
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
    BPTR cd = Lock(dr,SHARED_LOCK);
    execute_open_with_command(cd, NULL);
    if (cd) UnLock(cd);
}

/*******************************/

void wanderer_menufunc_wanderer_shell(STRPTR *cd_ptr)
{
    //TODO: remove the STRPTR *cdptr from top
    //TODO:remove this commented out stuff
    //BPTR cd = Lock(*cd_ptr,ACCESS_READ);
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
    BPTR cd = Lock(dr,ACCESS_READ);
    if (SystemTags("NewShell", NP_CurrentDir, (IPTR)cd, TAG_DONE) == -1)
    {
        UnLock(cd);
    }
}

void wanderer_menufunc_wanderer_backdrop(Object **pstrip)
{
	struct Wanderer_DATA *data = INST_DATA(_WandererIntern_CLASS, _WandererIntern_AppObj);
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WANDERER_BACKDROP);

D(bug("[WANDERER] wanderer_menufunc_wanderer_backdrop()\n"));

    if (item != NULL)
    {
		data->wd_Option_BackDropMode = (BOOL)XGET(item, MUIA_Menuitem_Checked);
	    SET(data->wd_WorkbenchWindow, MUIA_IconWindow_IsBackdrop, data->wd_Option_BackDropMode);
    }
}

void wanderer_menufunc_window_newdrawer(STRPTR *cdptr)
{
    //TODO: remove the STRPTR *cdptr from top
    
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
D(bug("[wanderer] wanderer_menufunc_window_newdrawer(%s)\n", dr));

    Object *actwindow = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *wbwindow = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_WorkbenchWindow);
    if (actwindow == wbwindow)
    {
        /* This check is necessary because WorkbenchWindow has path RAM: */
D(bug("[wanderer] wanderer_menufunc_window_newdrawer: Can't call WBNewDrawer for WorkbenchWindow\n"));
        return;
    }
    if ( XGET(actwindow, MUIA_Window_Open) == FALSE )
    {
D(bug("[wanderer] wanderer_menufunc_window_newdrawer: Can't call WBNewDrawer: the active window isn't open\n"));
        return;
    }

    BPTR lock = Lock(dr, ACCESS_READ);
    OpenWorkbenchObject
	(
        "WANDERER:Tools/WBNewDrawer",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, 0,
        TAG_DONE
	);
    UnLock(lock);
}

void wanderer_menufunc_window_openparent(STRPTR *cdptr)
{
    //TODO: Remove the **cdptr stuff from top
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
        
    IPTR	path_len=0;
	STRPTR	last_letter=NULL;
    last_letter = &dr[ strlen(dr) - 1 ];
	
	STRPTR thispath = FilePart(dr);
	
	if (*last_letter==0x3a) return; /* Top Drawer has no parent to open */
	
	last_letter = &thispath[strlen(thispath)-1];
	
	if (*last_letter==0x3a) 
       path_len = (IPTR)(thispath-(IPTR)(dr));
	else 
       path_len = (IPTR)((thispath-(IPTR)(dr))-1);
	
	STRPTR buf = AllocVec((path_len+1),MEMF_PUBLIC|MEMF_CLEAR);	
	CopyMem(dr, buf, path_len);
	
	Object *cstate = (Object*)(((struct List*)XGET(_WandererIntern_AppObj, MUIA_Application_WindowList))->lh_Head);
	Object *child;
	
    // Make sure we have a correct path   
    BOOL foundSlash = FALSE, foundColon = FALSE;
    int i = 0; for ( ; i < path_len; i++ )
    {
        if ( buf[ i ] == '/' ) foundSlash = TRUE;
        if ( buf[ i ] == ':' ) foundColon = TRUE;
    }
    if ( !foundColon && !foundSlash )
    {
        STRPTR newbuf = AllocVec ((path_len + 2), MEMF_PUBLIC|MEMF_CLEAR);
        sprintf(newbuf,"%s:",buf);
        FreeVec (buf);
        buf = newbuf;
    }
    // Done with path correction check
    
	while ((child = NextObject(&cstate)))
	{
		if (XGET(child, MUIA_UserData))
		{      
			STRPTR child_drawer = (STRPTR)XGET(child, MUIA_IconWindow_Location);
			if (child_drawer && !Stricmp(buf,child_drawer))
			{
				int is_open = XGET(child, MUIA_Window_Open);
				if (!is_open)
					DoMethod(child, MUIM_IconWindow_Open);
				else
				{
					DoMethod(child, MUIM_Window_ToFront);
					SET(child, MUIA_Window_Activate, TRUE);
				}
			    FreeVec(buf);
			    return; 
			}
		}
	}
	   
	DoMethod(_WandererIntern_AppObj, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
	FreeVec(buf);
}

void wanderer_menufunc_window_close()
{
	Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
	SET(window, MUIA_Window_CloseRequest, TRUE);
}

void wanderer_menufunc_window_update()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_Update);
    }
}

void wanderer_menufunc_window_clear()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_UnselectAll);
    }
}

void wanderer_menufunc_window_select()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_SelectAll);
    }
}

void wanderer_menufunc_window_view_icons(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_ALL);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
		ULONG display_bits = 0, menu_view_state = 0;
		GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);

		GET(item, MUIA_Menuitem_Checked, &menu_view_state);

		if (menu_view_state)
		{
			display_bits &= ~ICONLIST_DISP_SHOWINFO;
		}
		else
		{
			display_bits |= ICONLIST_DISP_SHOWINFO;
		}

        SET(iconList, MUIA_IconList_DisplayFlags, display_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_name(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		/*name = date and size bit both NOT set*/
		if( (sort_bits & ICONLIST_SORT_BY_DATE) || (sort_bits & ICONLIST_SORT_BY_SIZE) )
		{
			sort_bits &= ~(ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE);
		}

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_date(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		/*exclude size bit*/
		if( sort_bits & ICONLIST_SORT_BY_SIZE )
		{
			sort_bits &= ~ICONLIST_SORT_BY_SIZE;
		}

		sort_bits |= ICONLIST_SORT_BY_DATE;

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_size(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		/*exclude date bit*/
		if( sort_bits & ICONLIST_SORT_BY_DATE )
		{
			sort_bits &= ~ICONLIST_SORT_BY_DATE;
		}

		sort_bits |= ICONLIST_SORT_BY_SIZE;

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_type(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		/*type = both date and size bits set*/
		sort_bits |= (ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE);

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_reverse(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_REVERSE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		if( XGET(item, MUIA_Menuitem_Checked) )
		{
			sort_bits |= ICONLIST_SORT_REVERSE;
		}
		else
		{
			sort_bits &= ~ICONLIST_SORT_REVERSE;
		}

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_sort_topdrawers(Object **pstrip)
{
	Object *strip = *pstrip;
	Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_TOPDRAWERS);
	Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
	Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
		ULONG sort_bits = 0;
		GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

		if( XGET(item, MUIA_Menuitem_Checked) )
		{
			sort_bits &= !ICONLIST_SORT_DRAWERS_MIXED;
		}
		else
		{
			sort_bits |= ICONLIST_SORT_DRAWERS_MIXED;
		}

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_icon_open()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    DoMethod(window, MUIM_IconWindow_DoubleClicked);
}

void wanderer_menufunc_icon_rename(void)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
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
            
D(bug("[wanderer] wanderer_menufunc_icon_rename: selected = '%s'\n", entry->filename));
            
            OpenWorkbenchObject
            (
                "WANDERER:Tools/WBRename",
                WBOPENA_ArgLock, (IPTR) parent,
                WBOPENA_ArgName, (IPTR) FilePart(entry->filename),
                TAG_DONE
            );
            
D(bug("[wanderer] wanderer_menufunc_icon_rename: selected = '%s'\n", entry->filename));
            
            UnLock(parent);
        }
        else
        {
            break;
        }
    } while (TRUE);
}

void wanderer_menufunc_icon_information()
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
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

D(bug("[wanderer] wanderer_menufunc_icon_information: selected = '%s'\n", entry->filename));

            WBInfo(parent, entry->filename, NULL);

            UnLock(parent);
        }
        else
        {
            break;
        }
    } while (TRUE);
}

/* dispose the file copy display */

void DisposeCopyDisplay(struct MUIDisplayObjects *d) 
{
    if (d->copyApp) 
    {
        SET(d->win,MUIA_Window_Open,FALSE);
        MUI_DisposeObject(d->copyApp);
    }
}

/* create the file copy window */
BOOL CreateCopyDisplay(UWORD flags, struct MUIDisplayObjects *d) 
{
    BOOL    back = FALSE;
    
    Object  *group, *fromObject, *toObject, *fileTextObject, *fileLengthObject, *gaugeGroup;

    d->stopflag = 0; // will be set to 1 when clicking on stop, than the displayhook can tell actionDir() to stop copy 
    d->bytes = 0;
    d->numfiles = 0;
    d->action = flags;
    d->smallobjects = 0;
    d->copyApp = ApplicationObject,
        MUIA_Application_Title,     (IPTR)"CopyRequester",
        MUIA_Application_Base,      (IPTR)"WANDERER_COPY",
        SubWindow, (IPTR)(d->win = WindowObject,
            MUIA_Window_Title,          (IPTR)"Copy Filesystem",
            MUIA_Window_Activate,       TRUE,
            MUIA_Window_DepthGadget,    TRUE,
            MUIA_Window_DragBar,        TRUE,
            MUIA_Window_SizeGadget,     TRUE,
            MUIA_Window_AppWindow,      FALSE,
            MUIA_Window_CloseGadget,    FALSE,
            MUIA_Window_Borderless,     FALSE,
            MUIA_Window_TopEdge,        MUIV_Window_TopEdge_Centered,
            MUIA_Window_LeftEdge,       MUIV_Window_LeftEdge_Centered,
            MUIA_Window_Width,          MUIV_Window_Width_Visible(60),
            WindowContents, (group = VGroup,
                Child, (IPTR)(fromObject = TextObject,
                    InnerSpacing(8,2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                End),
                Child, (IPTR)(d->sourceObject = TextObject,
                    TextFrame,
                    InnerSpacing(8,2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                End),
                Child, (IPTR)(toObject = TextObject,
                    InnerSpacing(8,2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                End),
                Child, (IPTR)(d->destObject = TextObject,
                    TextFrame,
                    InnerSpacing(8,2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                End),
                Child, (IPTR)(fileTextObject = TextObject,
                    InnerSpacing(8,2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                End),
                Child, (IPTR)(d->fileObject = TextObject,
                    TextFrame,
                    InnerSpacing(8,2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                End),
                Child, (IPTR)(fileLengthObject = TextObject,
                    InnerSpacing(8,2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                End),
                Child, (IPTR)(gaugeGroup = VGroup,
                    TextFrame,
                    Child, d->gauge = GaugeObject,
                        MUIA_Gauge_Horiz, TRUE,
                        MUIA_Gauge_Max, 32768,
                        MUIA_Gauge_InfoText, "Processing...",
                    End,
                    Child, ScaleObject,
                        MUIA_Scale_Horiz, TRUE,
                    End,
                End),
                Child, (IPTR)( d->performanceObject = TextObject,
                    TextFrame,
                    InnerSpacing(8,2),
                    MUIA_Background,     MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"...........0 Bytes...........",
                End),

                Child, (IPTR)( d->stopObject = SimpleButton("Stop") ),
            End),
        End),
    End;


    if (d->copyApp) 
    {
        if ((flags & (ACTION_COPY|ACTION_DELETE)) == (ACTION_COPY|ACTION_DELETE)) 
        {
            SET(fromObject, MUIA_Text_Contents, (IPTR)"move from");
            SET(toObject, MUIA_Text_Contents, (IPTR)"move to");
            SET(fileTextObject, MUIA_Text_Contents, (IPTR)"file");
            SET(fileLengthObject, MUIA_Text_Contents, (IPTR)"traffic");
        } 
        else if ((flags & ACTION_COPY) == ACTION_COPY) 
        {
            SET(fromObject, MUIA_Text_Contents, (IPTR)"copy from");
            SET(toObject, MUIA_Text_Contents, (IPTR)"copy to");
            SET(fileTextObject, MUIA_Text_Contents, (IPTR)"file");
            SET(fileLengthObject, MUIA_Text_Contents, (IPTR)"traffic");

        } 
        else if ((flags & ACTION_DELETE) == ACTION_DELETE) 
        {
            SET(fromObject, MUIA_Text_Contents, "delete from");
            DoMethod(group, MUIM_Group_InitChange);
            DoMethod(group, OM_REMMEMBER, toObject);
            DoMethod(group, OM_REMMEMBER, fileLengthObject);
            DoMethod(group, OM_REMMEMBER, d->performanceObject);
            DoMethod(group, OM_REMMEMBER, d->destObject);
            DoMethod(group, OM_REMMEMBER, gaugeGroup);
            DoMethod(group, MUIM_Group_ExitChange);
            SET(fileTextObject, MUIA_Text_Contents, "file to delete");
        }

        SET(d->win,MUIA_Window_Open,TRUE);
        DoMethod(d->stopObject,MUIM_Notify, MUIA_Pressed, FALSE, d->stopObject, 3, MUIM_WriteLong, 1 ,&d->stopflag);
        back = TRUE;
    }
    return back;
}

void wanderer_menufunc_icon_delete(void)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = ( void*) MUIV_IconList_NextSelected_Start;
    DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
    
    struct MUIDisplayObjects dobjects;
    struct Hook displayCopyHook;
    struct Hook displayDelHook;
    displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
    displayDelHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskDeleteFunc;
    
    ULONG updatedIcons = 0;
    
    /* Process all selected entries */
    if (CreateCopyDisplay(ACTION_DELETE, &dobjects))
    {
        do
        {   
            if ((int)entry != MUIV_IconList_NextSelected_End) 
            {  
                /* copy via filesystems.c */
                D(bug("[WANDERER] Delete \"%s\"\n", entry->filename);)
                CopyContent( NULL, entry->filename, NULL, TRUE, ACTION_DELETE, &displayCopyHook, &displayDelHook, (APTR) &dobjects);
                updatedIcons++;
            }
            DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
        } 
        while ( (int)entry != MUIV_IconList_NextSelected_End );
        DisposeCopyDisplay(&dobjects);
    }
    // Only update list if anything happened to the icons!
    if ( updatedIcons > 0 )
    {
        DoMethod(window, MUIM_IconWindow_UnselectAll);
        DoMethod ( iconList, MUIM_IconList_Update );
    }
}

void wanderer_menufunc_wanderer_guisettings(void)
{
    //DoMethod(_WandererIntern_AppObj, MUIM_Application_OpenConfigWindow);
    OpenWorkbenchObject("SYS:Prefs/Zune",
                WBOPENA_ArgName, (IPTR) "WANDERER",
                TAG_DONE);
}

void wanderer_menufunc_wanderer_about(void)
{
    OpenWorkbenchObject("SYS:System/About", TAG_DONE);
}

void wanderer_menufunc_wanderer_quit(void)
{
    //if (MUI_RequestA(_WandererIntern_AppObj, NULL, 0, "Wanderer", _(MSG_YESNO), _(MSG_REALLYQUIT), NULL))
	//DoMethod(_WandererIntern_AppObj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    OpenWorkbenchObject("WANDERER:Tools/Quit", TAG_DONE);
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
            (IPTR) entry, 4, MUIM_CallHook, (IPTR) &_WandererIntern_hook_standard,
            (IPTR) function, (IPTR) arg
        );
    }
}

VOID DoAllMenuNotifies(Object *strip, STRPTR path)
{
    Object *item;

    if (!strip) return;

    DoMenuNotify(strip, MEN_WANDERER_EXECUTE,       wanderer_menufunc_wanderer_execute,       path);
    DoMenuNotify(strip, MEN_WANDERER_SHELL,         wanderer_menufunc_wanderer_shell,         path);
    DoMenuNotify(strip, MEN_WANDERER_GUISETTINGS,   wanderer_menufunc_wanderer_guisettings,   NULL);
    DoMenuNotify(strip, MEN_WANDERER_ABOUT,         wanderer_menufunc_wanderer_about,         NULL);
    DoMenuNotify(strip, MEN_WANDERER_QUIT,          wanderer_menufunc_wanderer_quit,          NULL);

    DoMenuNotify(strip, MEN_WINDOW_NEW_DRAWER,      wanderer_menufunc_window_newdrawer,       path);
    DoMenuNotify(strip, MEN_WINDOW_OPEN_PARENT,     wanderer_menufunc_window_openparent,      path);
    DoMenuNotify(strip, MEN_WINDOW_CLOSE,           wanderer_menufunc_window_close,           NULL);
    DoMenuNotify(strip, MEN_WINDOW_UPDATE,          wanderer_menufunc_window_update,          NULL);
    DoMenuNotify(strip, MEN_WINDOW_CLEAR,           wanderer_menufunc_window_clear,           NULL);
    DoMenuNotify(strip, MEN_WINDOW_SELECT,          wanderer_menufunc_window_select,          NULL);
    DoMenuNotify(strip, MEN_WINDOW_VIEW_ALL,        wanderer_menufunc_window_view_icons,      strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_NAME,       wanderer_menufunc_window_sort_name,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TYPE,       wanderer_menufunc_window_sort_type,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_DATE,       wanderer_menufunc_window_sort_date,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_SIZE,       wanderer_menufunc_window_sort_size,       strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_REVERSE,    wanderer_menufunc_window_sort_reverse,    strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TOPDRAWERS, wanderer_menufunc_window_sort_topdrawers, strip);

    DoMenuNotify(strip, MEN_ICON_OPEN,              wanderer_menufunc_icon_open,              NULL);
    DoMenuNotify(strip, MEN_ICON_RENAME,            wanderer_menufunc_icon_rename,            NULL);
    DoMenuNotify(strip, MEN_ICON_INFORMATION,       wanderer_menufunc_icon_information,       NULL);
    DoMenuNotify(strip, MEN_ICON_DELETE,            wanderer_menufunc_icon_delete,            NULL);
    
    if ((item = FindMenuitem(strip, MEN_WANDERER_BACKDROP)))
    {
	    DoMethod
        (
            item, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, 
            (IPTR) _WandererIntern_AppObj, 7, MUIM_Application_PushMethod, 
            (IPTR) _WandererIntern_AppObj, 4, MUIM_CallHook, (IPTR) &_WandererIntern_hook_standard, 
            (IPTR) wanderer_menufunc_wanderer_backdrop, (IPTR) strip
        );
    }
}

/*** Methods ****************************************************************/
Object *Wanderer__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
D(bug("[Wanderer] Wanderer__OM_NEW()\n"));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,
        
        MUIA_Application_Title,       (IPTR) "Wanderer",
	    MUIA_Application_Base,        (IPTR) "WANDERER",
	    MUIA_Application_Version,     (IPTR) VERSION,
	    MUIA_Application_Description, (IPTR) _(MSG_DESCRIPTION),
	    MUIA_Application_SingleTask,         TRUE,
        
        TAG_MORE, (IPTR) message->ops_AttrList
    );
    
    if (self != NULL)
    {
        SETUP_WANDERER_INST_DATA;

D(bug("[Wanderer] Wanderer__OM_NEW: SELF = %d, Private data @ %x\n", self, data));

		data->wd_Screen = LockPubScreen(NULL);
		_WandererIntern_CLASS = CLASS;

#if defined(WANDERER_DEFAULT_BACKDROP)
		data->wd_Option_BackDropMode = TRUE;
#else
		data->wd_Option_BackDropMode = FALSE;
#endif

		if(data->wd_Screen == NULL)
		{
D(bug("[Wanderer] Wanderer__OM_NEW: Couldn't lock screen!\n"));
            CoerceMethod(CLASS, self, OM_DISPOSE);
            return NULL;
		}

D(bug("[Wanderer] Wanderer__OM_NEW: Using Screen @ %x\n", data->wd_Screen));

        /*-- Setup hooks structures ----------------------------------------*/
        _WandererIntern_hook_standard.h_Entry = (HOOKFUNC) Wanderer__HookFunc_StandardFunc;
        _WandererIntern_hook_action.h_Entry   = (HOOKFUNC) Wanderer__HookFunc_ActionFunc;
	_WandererIntern_hook_backdrop.h_Entry   = (HOOKFUNC) Wanderer__HookFunc_BackdropFunc;

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

        // All the following should be moved to InitWandererPrefs

        /* Setup notification on prefs file --------------------------------*/
        data->pnr.nr_Name                 = ExpandEnvName("ENV:SYS/Wanderer.prefs");
        data->pnr.nr_Flags                = NRF_SEND_MESSAGE;
        data->pnr.nr_stuff.nr_Msg.nr_Port = data->wd_NotifyPort;

        if (StartNotify(&data->pnr))
        {
D(bug("[Wanderer] Wanderer__OM_NEW: Prefs-notification setup on '%s'\n", data->pnr.nr_Name));
        }
        else
        {
D(bug("[Wanderer] Wanderer__OM_NEW: FAILED to setup Prefs-notification!\n"));
        }

        data->wd_Prefs = WandererPrefsObject, End; // FIXME: error handling

		if (data->wd_Prefs) 
			data->wd_PrefsIntern = InitWandererPrefs();
    }

    return self;
}

IPTR Wanderer__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_WANDERER_INST_DATA;
    
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
    SETUP_WANDERER_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((const struct TagItem**)&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
		case MUIA_Wanderer_Screen:
D(bug("[Wanderer] Wanderer__OM_SET: MUIA_Wanderer_Screen = %p\n", tag->ti_Data));
D(bug("[Wanderer] Wanderer__OM_SET: setting MUIA_Wanderer_Screen isnt yet handled!\n"));
                break;

		case MUIA_Wanderer_ActiveWindow:
                data->wd_ActiveWindow = (Object *) tag->ti_Data;
D(bug("[Wanderer] Wanderer__OM_SET: MUIA_Wanderer_ActiveWindow = %p\n", tag->ti_Data));
				if (!(XGET(data->wd_ActiveWindow, MUIA_Window_Activate)))
					NNSET(data->wd_ActiveWindow, MUIA_Window_Activate, TRUE);

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
    SETUP_WANDERER_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;
    
    switch (message->opg_AttrID)
    {
		case MUIA_Wanderer_Screen:
            *store = (IPTR) data->wd_Screen;
			break;

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

/* Main entry point for Wanderer Application Object */
/*
	When the executable creates our object it calls zune 
    to handle basic "control" ... which results in Zune 
    calling this method ..
*/
IPTR Wanderer__MUIM_Application_Execute
(
    Class *CLASS, Object *self, Msg message 
)
{
    SETUP_WANDERER_INST_DATA;
    
D(bug("[Wanderer] Wanderer__MUIM_Application_Execute() ##\n[Wanderer] Wanderer__MUIM_Application_Execute: Creating 'Workbench' Window..\n"));

    data->wd_WorkbenchWindow = (Object *) DoMethod
    (
        self, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
    );
    
    if (data->wd_WorkbenchWindow != NULL)
    {
D(bug("[Wanderer] Wanderer__MUIM_Application_Execute: Workbench Window Obj @ %x\n", data->wd_WorkbenchWindow));

        Detach();

D(bug("[Wanderer] Wanderer__MUIM_Application_Execute: Really handing control to Zune ..\n"));

	    DoSuperMethodA(CLASS, self, message);
        
        return RETURN_OK;
    }
    
#warning "TODO: Report an error if we fail to create the Workbench's window ..."
    
    return RETURN_ERROR;
}

/*This function uses GetScreenTitle() function...*/

IPTR Wanderer__MUIM_Wanderer_HandleTimer
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WANDERER_INST_DATA;
    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
    Object *child = NULL;
    STRPTR scr_title = GetScreenTitle();

    while ((child = NextObject(&cstate)))
	   SET(child, MUIA_Window_ScreenTitle, (IPTR) scr_title);
    
    return TRUE;
}


IPTR Wanderer__MUIM_Wanderer_HandleCommand
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WANDERER_INST_DATA;
    struct WBHandlerMessage *wbhm = NULL;
    
D(bug("[Wanderer] Recieved signal at notify port\n"));
    
    while ((wbhm = WBHM(GetMsg(data->wd_CommandPort))) != NULL)
    {
D(bug("[Wanderer] Recieved message from handler, type = %ld\n", wbhm->wbhm_Type));
        
        switch (wbhm->wbhm_Type)
        {
            case WBHM_TYPE_SHOW:
D(bug("[Wanderer] WBHM_TYPE_SHOW\n"));
                SET(self, MUIA_ShowMe, TRUE);
                break;
            
            case WBHM_TYPE_HIDE:
D(bug("[Wanderer] WBHM_TYPE_HIDE\n"));
                SET(self, MUIA_ShowMe, FALSE);
                break;
                
            case WBHM_TYPE_UPDATE:
D(bug("[Wanderer] WBHM_TYPE_UPDATE\n"));
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
                    
D(bug("[Wanderer] name = %s, length = %ld\n", name, length));
                    
                    {
                        Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                        Object *child = NULL;
                        
                        while ((child = NextObject(&cstate)))
                        {
                            if (XGET(child, MUIA_UserData))
                            {
                                STRPTR child_drawer = (STRPTR)XGET(child, MUIA_IconWindow_Location);
                                
                                if
                                (
                                    child_drawer != NULL 
                                    && strncmp(name, child_drawer, length) == 0
                                    && strlen(child_drawer) == length
                                )
                                {
                                    Object *iconlist = (Object *) XGET(child, MUIA_IconWindow_IconList);
                                    
D(bug("[Wanderer] Drawer found: %s!\n", child_drawer));
                                    
                                    if (iconlist != NULL)
                                    {
                                        DoMethod ( iconlist, MUIM_IconList_Update );
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                break;
                
            case WBHM_TYPE_OPEN:
D(bug("[Wanderer] WBHM_TYPE_OPEN\n"));
            
                {
                    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
                    Object *child;
                    CONST_STRPTR buf = wbhm->wbhm_Data.Open.Name;
                    
                    while ((child = NextObject(&cstate)))
                    {
                        if (XGET(child, MUIA_UserData))
                        {
                            STRPTR child_drawer = (STRPTR)XGET(child, MUIA_IconWindow_Location);
                            if (child_drawer && !Stricmp(buf,child_drawer))
                            {
                                int is_open = XGET(child, MUIA_Window_Open);
                                if (!is_open)
                                    DoMethod(child, MUIM_IconWindow_Open);
                                else
                                {
                                    DoMethod(child, MUIM_Window_ToFront);
                                    SET(child, MUIA_Window_Activate, TRUE);
                                }
                                return 0; 
                            }
                        }
                    }
                    
                    DoMethod
                    (
                        _WandererIntern_AppObj, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf
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
    SETUP_WANDERER_INST_DATA;
    struct Message *notifyMessage = NULL;
    
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_HandleNotify: got prefs change notification!\n"));
    
    while ((notifyMessage = GetMsg(data->wd_NotifyPort)) != NULL)
    {
        ReplyMsg(notifyMessage);
    }
    
    /* reload prefs file */
    DoMethod(data->wd_Prefs, MUIM_WandererPrefs_Reload);

    return 0;
}

/* Some differences here between volumes and subwindows */
Object * __CreateWandererIntuitionMenu__ ( BOOL isRoot, BOOL isBackdrop)
{
    Object   *_NewWandIntMenu__menustrip = NULL;
	IPTR     _NewWandIntMenu__OPTION_BACKDROP = CHECKIT|MENUTOGGLE;
	IPTR     _NewWandIntMenu__OPTION_SHOWALL  = CHECKIT|MENUTOGGLE;

	if (isBackdrop)
	{
		_NewWandIntMenu__OPTION_BACKDROP |= CHECKED;
	}

#if defined(WANDERER_DEFAULT_SHOWALL)
	_NewWandIntMenu__OPTION_SHOWALL |= CHECKED;
#endif

    if ( isRoot )
    {
        struct NewMenu nm[] = {
        {NM_TITLE,     _(MSG_MEN_WANDERER)},
            {NM_ITEM,  _(MSG_MEN_BACKDROP),_(MSG_MEN_SC_BACKDROP), _NewWandIntMenu__OPTION_BACKDROP, 0, (APTR) MEN_WANDERER_BACKDROP},
            {NM_ITEM,  _(MSG_MEN_EXECUTE), _(MSG_MEN_SC_EXECUTE) , 0                         , 0, (APTR) MEN_WANDERER_EXECUTE},
    
            {NM_ITEM,  _(MSG_MEN_SHELL),   _(MSG_MEN_SC_SHELL)   , 0                         , 0, (APTR) MEN_WANDERER_SHELL},
            {NM_ITEM,  _(MSG_MEN_GUISET),  NULL                  , 0                         , 0, (APTR) MEN_WANDERER_GUISETTINGS},
            {NM_ITEM,  _(MSG_MEN_ABOUT),   _(MSG_MEN_SC_ABOUT)   , 0                         , 0, (APTR) MEN_WANDERER_ABOUT},
            {NM_ITEM,  _(MSG_MEN_QUIT) ,   _(MSG_MEN_SC_QUIT)    , 0                         , 0, (APTR) MEN_WANDERER_QUIT},
    
        {NM_TITLE,     _(MSG_MEN_WINDOW),  NULL, NM_MENUDISABLED},
    
            {NM_ITEM,  _(MSG_MEN_UPDATE),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_UPDATE},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM, _(MSG_MEN_CONTENTS), _(MSG_MEN_SC_CONTENTS), 0                         , 0, (APTR) MEN_WINDOW_SELECT},
            {NM_ITEM,  _(MSG_MEN_CLRSEL),  _(MSG_MEN_SC_CLRSEL)  , 0                         , 0, (APTR) MEN_WINDOW_CLEAR},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_SNAPSHT) },
            {NM_SUB,   _(MSG_MEN_WINDOW),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_WIN},
            {NM_SUB,   _(MSG_MEN_ALL),     NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_ALL},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_VIEW)},
            {NM_SUB,   _(MSG_MEN_ICVIEW),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_VIEW_ICON},
            {NM_SUB,   _(MSG_MEN_DCVIEW),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_VIEW_DETAIL},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,   _(MSG_MEN_ALLFIL),  NULL                  , _NewWandIntMenu__OPTION_SHOWALL, 0, (APTR) MEN_WINDOW_VIEW_ALL},
            {NM_ITEM,  _(MSG_MEN_SORTIC)},
            {NM_SUB,   _(MSG_MEN_CLNUP),   _(MSG_MEN_SC_CLNUP)   , 0                         , 0, (APTR) MEN_WINDOW_SORT_NOW},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,   _(MSG_MEN_BYNAME),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_SORT_NAME},
            {NM_SUB,   _(MSG_MEN_BYDATE),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_SORT_DATE},
            {NM_SUB,   _(MSG_MEN_BYSIZE),  NULL                  , CHECKIT               ,4+8+32, (APTR) MEN_WINDOW_SORT_SIZE},
        //{NM_SUB,   "..by Type",           NULL, CHECKIT,4+8+16, (APTR) MEN_WINDOW_SORT_TYPE},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,  _(MSG_MEN_REVERSE),  NULL                  , CHECKIT|MENUTOGGLE        , 0, (APTR) MEN_WINDOW_SORT_REVERSE},
            {NM_SUB,  _(MSG_MEN_DRWFRST),  NULL                  , CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_TOPDRAWERS},
        //{NM_SUB,  "Group Icons",           NULL, CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_GROUP},
    
        {NM_TITLE,    _(MSG_MEN_ICON),     NULL, NM_MENUDISABLED},
            {NM_ITEM,  _(MSG_MEN_OPEN), _(MSG_MEN_SC_OPEN), 0, 0, (APTR) MEN_ICON_OPEN},
    //    {NM_ITEM,  "Close","C" },
            {NM_ITEM,  _(MSG_MEN_RENAME), _(MSG_MEN_SC_RENAME), 0, 0, (APTR) MEN_ICON_RENAME},
            {NM_ITEM,  _(MSG_MEN_INFO), _(MSG_MEN_SC_INFO), 0, 0, (APTR) MEN_ICON_INFORMATION},
    //    {NM_ITEM,  "Snapshot", "S" },
    //    {NM_ITEM,  "Unsnapshot", "U" },
    //    {NM_ITEM,  "Leave Out", "L" },
    //    {NM_ITEM,  "Put Away", "P" },
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_DELETE), NULL, 0, 0, (APTR) MEN_ICON_DELETE},
    //    {NM_ITEM,  "Format Disk..." },
    //    {NM_ITEM,  "Empty Trash..." },
    
        {NM_TITLE, _(MSG_MEN_TOOLS),          NULL, NM_MENUDISABLED},
    //    {NM_ITEM,  "ResetWanderer" },
        {NM_END}
        };
        _NewWandIntMenu__menustrip = MUI_MakeObject(MUIO_MenustripNM, nm, (IPTR) NULL);
    }
    else
    {
        struct NewMenu nm[] = {
        {NM_TITLE,     _(MSG_MEN_WANDERER)},
            {NM_ITEM,  _(MSG_MEN_BACKDROP),_(MSG_MEN_SC_BACKDROP), _NewWandIntMenu__OPTION_BACKDROP, 0, (APTR) MEN_WANDERER_BACKDROP},
            {NM_ITEM,  _(MSG_MEN_EXECUTE), _(MSG_MEN_SC_EXECUTE) , 0                         , 0, (APTR) MEN_WANDERER_EXECUTE},
    
            {NM_ITEM,  _(MSG_MEN_SHELL),   _(MSG_MEN_SC_SHELL)   , 0                         , 0, (APTR) MEN_WANDERER_SHELL},
            {NM_ITEM,  _(MSG_MEN_GUISET),  NULL                  , 0                         , 0, (APTR) MEN_WANDERER_GUISETTINGS},
            {NM_ITEM,  _(MSG_MEN_ABOUT),   _(MSG_MEN_SC_ABOUT)   , 0                         , 0, (APTR) MEN_WANDERER_ABOUT},
            {NM_ITEM,  _(MSG_MEN_QUIT) ,   _(MSG_MEN_SC_QUIT)    , 0                         , 0, (APTR) MEN_WANDERER_QUIT},
    
        {NM_TITLE,     _(MSG_MEN_WINDOW),  NULL, NM_MENUDISABLED},
    
            {NM_ITEM,  _(MSG_MEN_NEWDRAW), _(MSG_MEN_SC_NEWDRAW) , 0                         , 0, (APTR) MEN_WINDOW_NEW_DRAWER},
            {NM_ITEM,  _(MSG_MEN_OPENPAR),  NULL                 , 0                         , 0, (APTR) MEN_WINDOW_OPEN_PARENT},
            {NM_ITEM,  _(MSG_MEN_CLOSE),   _(MSG_MEN_SC_CLOSE)   , 0                         , 0, (APTR) MEN_WINDOW_CLOSE},
            {NM_ITEM,  _(MSG_MEN_UPDATE),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_UPDATE},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM, _(MSG_MEN_CONTENTS), _(MSG_MEN_SC_CONTENTS), 0                         , 0, (APTR) MEN_WINDOW_SELECT},
            {NM_ITEM,  _(MSG_MEN_CLRSEL),  _(MSG_MEN_SC_CLRSEL)  , 0                         , 0, (APTR) MEN_WINDOW_CLEAR},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_SNAPSHT) },
            {NM_SUB,   _(MSG_MEN_WINDOW),  NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_WIN},
            {NM_SUB,   _(MSG_MEN_ALL),     NULL                  , 0                         , 0, (APTR) MEN_WINDOW_SNAP_ALL},
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_VIEW)},
            {NM_SUB,   _(MSG_MEN_ICVIEW),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_VIEW_ICON},
            {NM_SUB,   _(MSG_MEN_DCVIEW),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_VIEW_DETAIL},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,   _(MSG_MEN_ALLFIL),  NULL                  , _NewWandIntMenu__OPTION_SHOWALL, 0, (APTR) MEN_WINDOW_VIEW_ALL},
            {NM_ITEM,  _(MSG_MEN_SORTIC)},
            {NM_SUB,   _(MSG_MEN_CLNUP),   _(MSG_MEN_SC_CLNUP)   , 0                         , 0, (APTR) MEN_WINDOW_SORT_NOW},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,   _(MSG_MEN_BYNAME),  NULL                  , CHECKIT|CHECKED      ,8+16+32, (APTR) MEN_WINDOW_SORT_NAME},
            {NM_SUB,   _(MSG_MEN_BYDATE),  NULL                  , CHECKIT              ,4+16+32, (APTR) MEN_WINDOW_SORT_DATE},
            {NM_SUB,   _(MSG_MEN_BYSIZE),  NULL                  , CHECKIT               ,4+8+32, (APTR) MEN_WINDOW_SORT_SIZE},
        //{NM_SUB,   "..by Type",           NULL, CHECKIT,4+8+16, (APTR) MEN_WINDOW_SORT_TYPE},
            {NM_SUB, NM_BARLABEL},
            {NM_SUB,  _(MSG_MEN_REVERSE),  NULL                  , CHECKIT|MENUTOGGLE        , 0, (APTR) MEN_WINDOW_SORT_REVERSE},
            {NM_SUB,  _(MSG_MEN_DRWFRST),  NULL                  , CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_TOPDRAWERS},
        //{NM_SUB,  "Group Icons",           NULL, CHECKIT|MENUTOGGLE|CHECKED, 0, (APTR) MEN_WINDOW_SORT_GROUP},
    
        {NM_TITLE,    _(MSG_MEN_ICON),     NULL, NM_MENUDISABLED},
            {NM_ITEM,  _(MSG_MEN_OPEN), _(MSG_MEN_SC_OPEN), 0, 0, (APTR) MEN_ICON_OPEN},
    //    {NM_ITEM,  "Close","C" },
            {NM_ITEM,  _(MSG_MEN_RENAME), _(MSG_MEN_SC_RENAME), 0, 0, (APTR) MEN_ICON_RENAME},
            {NM_ITEM,  _(MSG_MEN_INFO), _(MSG_MEN_SC_INFO), 0, 0, (APTR) MEN_ICON_INFORMATION},
    //    {NM_ITEM,  "Snapshot", "S" },
    //    {NM_ITEM,  "Unsnapshot", "U" },
    //    {NM_ITEM,  "Leave Out", "L" },
    //    {NM_ITEM,  "Put Away", "P" },
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_DELETE), NULL, 0, 0, (APTR) MEN_ICON_DELETE},
    //    {NM_ITEM,  "Format Disk..." },
    //    {NM_ITEM,  "Empty Trash..." },
    
        {NM_TITLE, _(MSG_MEN_TOOLS),          NULL, NM_MENUDISABLED},
    //    {NM_ITEM,  "ResetWanderer" },
        {NM_END}
        };
        _NewWandIntMenu__menustrip = MUI_MakeObject(MUIO_MenustripNM, nm, (IPTR) NULL);
    }
    return _NewWandIntMenu__menustrip;
}

Object *Wanderer__MUIM_Wanderer_CreateDrawerWindow
(
    Class *CLASS, Object *self, 
    struct MUIP_Wanderer_CreateDrawerWindow *message
)
{
    SETUP_WANDERER_INST_DATA;

D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow()\n"));

    Object *window                = NULL;
    BOOL    isWorkbenchWindow     = FALSE;
    BOOL    useBackdrop           = FALSE;

	if (isWorkbenchWindow = (message->drawer == NULL ? TRUE : FALSE))
	{
		useBackdrop = data->wd_Option_BackDropMode;
	}

    BOOL    hasToolbar            = XGET(data->wd_Prefs, MUIA_IconWindowExt_Toolbar_Enabled);

    IPTR    TAG_IconWindow_Drawer = isWorkbenchWindow ? TAG_IGNORE : MUIA_IconWindow_Location;

    IPTR    useFont = (IPTR)NULL;
    if (data->wd_PrefsIntern)
    {
        useFont = (IPTR)((struct WandererInternalPrefsData *)data->wd_PrefsIntern)->WIPD_IconFont;
    }

    Object *_NewWandDrawerMenu__menustrip = __CreateWandererIntuitionMenu__ (isWorkbenchWindow, useBackdrop);
    
    /* Create a new icon drawer window with the correct drawer being set */
    window = IconWindowObject,
        		MUIA_UserData, 			      1,
			MUIA_Wanderer_Prefs,                  data->wd_Prefs,
			MUIA_Wanderer_Screen,                 data->wd_Screen,
        		MUIA_Window_ScreenTitle,              GetScreenTitle(),
        		MUIA_Window_Menustrip,                (IPTR) _NewWandDrawerMenu__menustrip,
			TAG_IconWindow_Drawer,                (IPTR) message->drawer,
        		MUIA_IconWindow_Font,                 useFont,
        		MUIA_IconWindow_ActionHook,           (IPTR) &_WandererIntern_hook_action,
        		MUIA_IconWindow_IsRoot,               isWorkbenchWindow ? TRUE : FALSE,
			isWorkbenchWindow ? MUIA_IconWindow_IsBackdrop : TAG_IGNORE, useBackdrop,
        		MUIA_Window_IsSubWindow,              isWorkbenchWindow ? FALSE : TRUE,
        		MUIA_IconWindowExt_Toolbar_Enabled,   hasToolbar ? TRUE : FALSE,
    	     End;
    
    if (window != NULL)
    {
        /* Get the drawer path back so we can use it also outside this function */
        STRPTR drw = NULL;
        BOOL freeDrwStr = FALSE;
        
        if (!isWorkbenchWindow) drw = (STRPTR) XGET(window, MUIA_IconWindow_Location);
        else
        {
            drw = AllocVec ( 5, MEMF_CLEAR );
            sprintf ( drw, "RAM:" );
            freeDrwStr = TRUE;    
        }
        
		if (isWorkbenchWindow)
		{
			DoMethod
			(
				window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
				(IPTR)self, 3, MUIM_CallHook, (IPTR)&_WandererIntern_hook_standard, (IPTR)wanderer_menufunc_wanderer_quit
			);
		}
		else
		{
			DoMethod
			(
				window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
				(IPTR)window, 1, MUIM_IconWindow_Remove
			);
		}

#if defined(WANDERER_DEFAULT_SHOWALL) || defined(WANDERER_DEFAULT_SHOWHIDDEN)
		Object *window_IconList = NULL;
		ULONG  current_DispFlags = 0;

		GET(window, MUIA_IconWindow_IconList, &window_IconList);

D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: IconWindows IconList @ %x\n", window_IconList));

		if (window_IconList != NULL)
		{
			GET(window_IconList, MUIA_IconList_DisplayFlags, &current_DispFlags);

D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Old Flags : %x\n", current_DispFlags));

#if defined(WANDERER_DEFAULT_SHOWALL)
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Telling IconList to Show 'ALL' Files\n"));
			current_DispFlags &= ~ICONLIST_DISP_SHOWINFO;
#endif
#if defined(WANDERER_DEFAULT_SHOWHIDDEN)
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Telling IconList to Show 'Hidden' Files\n"));
			current_DispFlags |= ICONLIST_DISP_SHOWHIDDEN;
#endif
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: New Flags : %x\n", current_DispFlags));
			SET(window_IconList, MUIA_IconList_DisplayFlags, current_DispFlags);
		}
#endif
		
        DoMethod
        (
            window, MUIM_Notify, MUIA_Window_Activate, TRUE,
            (IPTR)_app(self), 3, MUIM_Set, MUIA_Wanderer_ActiveWindow, (IPTR) window
        );

#if 1
        DoMethod
        (
            window, MUIM_Notify, MUIA_IconWindow_IsBackdrop, MUIV_EveryTime,
            (IPTR)_app(self), 5, MUIM_Application_PushMethod, (IPTR)_app(self), 2, MUIM_CallHook, (IPTR)&_WandererIntern_hook_backdrop
        );
#else
        DoMethod
        (
            window, MUIM_Notify, MUIA_IconWindow_IsBackdrop, MUIV_EveryTime,
            (IPTR)_app(self), 2, MUIM_CallHook, (IPTR) &_WandererIntern_hook_backdrop
        );
#endif

        /* If "Execute Command" entry is clicked open the execute window */
        DoAllMenuNotifies(_NewWandDrawerMenu__menustrip, drw);        
        
        /* Add the window to the application */
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) window);
        
        /* And now open it */
        DoMethod(window, MUIM_IconWindow_Open);

        /* Clean up ram string */
        if ( freeDrwStr && drw ) FreeVec ( drw );
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
