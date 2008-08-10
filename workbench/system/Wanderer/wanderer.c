/*
    Copyright  2004-2008, The AROS Development Team. All rights reserved.
    $Id$
*/
#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 1
#include <aros/debug.h>
#endif

#define WANDERER_DEFAULT_BACKDROP
//#define WANDERER_DEFAULT_SHOWALL
//#define WANDERER_DEFAULT_SHOWHIDDEN

#include <exec/types.h>
#include <libraries/gadtools.h>
#include <libraries/mui.h>

#ifdef __AROS__
#include <zune/customclasses.h>
#else
#include <zune_AROS/customclasses.h>
#endif

#include <dos/notify.h>

#ifdef __AROS__
#include <workbench/handler.h>
#else
#include <workbench_AROS/handler.h>
#endif

#include <proto/graphics.h>
#include <proto/utility.h>

#include <proto/dos.h>

#include <proto/icon.h>

#ifdef __AROS__
#include <proto/workbench.h>
#endif

#include <proto/layers.h>

#ifdef __AROS__
#include <proto/alib.h>
#endif

#include <string.h>
#include <stdio.h>
#include <time.h>


#ifdef __AROS__
#include <aros/detach.h>
#include <prefs/wanderer.h>
#else
#include <prefs_AROS/wanderer.h>
#endif

#if defined(__AMIGA__) && !defined(__PPC__)
#define NO_INLINE_STDARG
#endif
#include <proto/intuition.h>
#include <proto/muimaster.h>


#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindowcontents.h"
#include "wandererprefs.h"
#include "wandererprefsintern.h"
#include "filesystems.h"
#include "wanderer.h"
#include "Classes/iconlist.h"
#include "Classes/iconlist_attributes.h"
#include "locale.h"

#include "version.h"

#ifndef __AROS__
#define DEBUG 1

#ifdef DEBUG
  #define D(x) if (DEBUG) x
  #ifdef __amigaos4__
  #define bug DebugPrintF
  #else
  #define bug kprintf
  #endif
#else
  #define  D(...)
#endif
#endif

#ifndef NO_ICON_POSITION
#define NO_ICON_POSITION                               (0x8000000) /* belongs to workbench/workbench.h */
#endif

#define KeyButton(name,key) TextObject, ButtonFrame, MUIA_Font, MUIV_Font_Button, MUIA_Text_Contents, (IPTR)(name), MUIA_Text_PreParse, "\33c", MUIA_Text_HiChar, (IPTR)(key), MUIA_ControlChar, key, MUIA_InputMode, MUIV_InputMode_RelVerify, MUIA_Background, MUII_ButtonBack, TAG_DONE)

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
extern Class     *_WandererIntern_CLASS;
/* Internal Hooks */
#ifdef __AROS__
struct Hook          _WandererIntern_hook_standard;
struct Hook          _WandererIntern_hook_action;
struct Hook          _WandererIntern_hook_backdrop;
#else
struct Hook          *_WandererIntern_hook_standard;
struct Hook          *_WandererIntern_hook_action;
struct Hook          *_WandererIntern_hook_backdrop;
#endif


static unsigned char strtochar(STRPTR st)
{
    return *st++;
}

/*** Instance Data **********************************************************/
struct Wanderer_DATA
{
    struct Screen         *wd_Screen;

    Object                      *wd_Prefs,
                                *wd_ActiveWindow,
                                *wd_WorkbenchWindow;

    struct MUI_InputHandlerNode  wd_TimerIHN;
    struct MsgPort              *wd_CommandPort;
    struct MUI_InputHandlerNode  wd_CommandIHN;
    struct MsgPort              *wd_NotifyPort;
    struct MUI_InputHandlerNode  wd_NotifyIHN;
    struct NotifyRequest         wd_PrefsNotifyRequest;

    IPTR                         wd_PrefsIntern;
    BOOL                         wd_Option_BackDropMode;
};

/*** Macros *****************************************************************/
#define SETUP_WANDERER_INST_DATA struct Wanderer_DATA *data = INST_DATA(CLASS, self)

/**************************************************************************
* HOOK FUNCS                                                              *
**************************************************************************/
///Wanderer__HookFunc_DisplayCopyFunc()
#ifdef __AROS__
AROS_UFH3
(
    BOOL, Wanderer__HookFunc_DisplayCopyFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct dCopyStruct *, obj, A2),
    AROS_UFHA(APTR, unused_param, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_DisplayCopyFunc, BOOL, struct dCopyStruct *obj, APTR unused_param)
{
#endif
    AROS_USERFUNC_INIT

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
#ifndef __AROS__
MakeStaticHook(Hook_DisplayCopyFunc,Wanderer__HookFunc_DisplayCopyFunc);
#endif
///

///Wanderer__HookFunc_AskDeleteFunc()
#ifdef __AROS__
AROS_UFH3
(
    ULONG, Wanderer__HookFunc_AskDeleteFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct dCopyStruct *, obj, A2),
    AROS_UFHA(APTR, unused_param, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_AskDeleteFunc, ULONG, struct dCopyStruct *obj, APTR unused_param)
{
#endif
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
#ifndef __AROS__
MakeStaticHook(Hook_AskDeleteFunc,Wanderer__HookFunc_AskDeleteFunc);
#endif
///

///copy_dropentries()
void copy_dropentries()
{
    /* get filelist from user message */
    struct Wanderer_FilelistMsg *message_filelist = FindTask(NULL)->tc_UserData;

    D(bug("[WANDERER COPY] CopyContent \n" ));

    if (message_filelist)
    {
           
        struct MUIDisplayObjects dobjects;
        struct Wanderer_FileEntry *currententry;
        #ifdef __AROS__
        struct Hook displayCopyHook;
        struct Hook displayDelHook;
        #else
        struct Hook *displayCopyHook;
        struct Hook *displayDelHook;
        #endif

        #ifdef __AROS__
        displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
        displayDelHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskDeleteFunc;
        #else
        displayCopyHook = &Hook_DisplayCopyFunc;
        displayDelHook = &Hook_AskDeleteFunc;
        #endif
        

        if (CreateCopyDisplay(ACTION_COPY, &dobjects))
        {

            /* process all selected entries */
            while ( (currententry = (struct Wanderer_FileEntry *)RemTail(&message_filelist->files)) != NULL )
            {

                /* copy via filesystems.c */
                D(bug("[WANDERER COPY] CopyContent \"%s\" to \"%s\"\n", &currententry->filename, 
                                    &message_filelist->destination_string ));

                CopyContent(NULL, (char*)&currententry->filename, (char*)&message_filelist->destination_string, TRUE, 
                ACTION_COPY, &displayCopyHook, &displayDelHook, (APTR) &dobjects);

                FreeVec( currententry );
            } 
            
            /* delete copy window */
            DisposeCopyDisplay(&dobjects);
        }

    }

    /* free msg memory */
    FreeMem( message_filelist, sizeof(struct Wanderer_FilelistMsg) );

    return;
}
///

///Wanderer__HookFunc_ActionFunc()
#ifdef __AROS__
AROS_UFH3
(
    void, Wanderer__HookFunc_ActionFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(Object *, obj, A2),
    AROS_UFHA(struct IconWindow_ActionMsg *, msg, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_ActionFunc, void, Object *obj, struct IconWindow_ActionMsg *msg)
{
#endif
    AROS_USERFUNC_INIT

    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
        static unsigned char  buf[1024];
  IPTR                  offset;
        struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;

        DoMethod(msg->iconlist, MUIM_IconList_NextSelected, (IPTR)&ent);
        if ((IPTR)ent == (IPTR)MUIV_IconList_NextSelected_End)
        {
D(bug("[WANDERER] Wanderer__HookFunc_ActionFunc: ICONWINDOW_ACTION_OPEN: NextSelected returned MUIV_IconList_NextSelected_TAG_DONE)\n"));
            return;
        }

        offset = strlen(ent->filename) - 5;

        if ((msg->isroot) && (!Stricmp(ent->filename + offset, ":Disk")))
        {
            strcpy((STRPTR)buf, ent->label);
            strcat((STRPTR)buf, ":");
        }
        else
        {
            strcpy((STRPTR)buf, ent->filename);
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
                        if (child_drawer && !Stricmp(buf,(CONST_STRPTR)child_drawer))
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
    struct Process *child;

    struct IconList_Drop *drop = (struct IconList_Drop *)msg->drop;
        struct IconList_Entry *ent = (void*)MUIV_IconList_NextSelected_Start;
    struct Wanderer_FileEntry *file_recordtmp;

    struct Wanderer_FilelistMsg *message_filelist = AllocMem( sizeof(struct Wanderer_FilelistMsg), MEMF_CLEAR|MEMF_PUBLIC );

    if (message_filelist != NULL)
    {
        strcpy( (char*)&message_filelist->destination_string,(STRPTR) drop->destination_string);
  
  #ifdef __AROS__
        NEWLIST(&message_filelist->files);
  #else
        NEW_LIST(&message_filelist->files);
  #endif
            /* process all selected entries */
            do
            {
                    DoMethod(drop->source_iconlistobj, MUIM_IconList_NextSelected, (IPTR) &ent);

                    /* if not end of selection, process */
                    if ( (int)ent != MUIV_IconList_NextSelected_End)
                    {
                file_recordtmp = AllocVec( sizeof(struct Wanderer_FileEntry), MEMF_CLEAR|MEMF_PUBLIC );
                strcpy( (char*)&file_recordtmp->filename, ent->filename);
                AddTail(&message_filelist->files, (struct Node *)file_recordtmp);
                    }
            } 
            while ( (int)ent != MUIV_IconList_NextSelected_End );

  {
        /* create process and copy files within */
          const struct TagItem       tags[]=
          {
                {NP_Entry    , (IPTR)copy_dropentries       },
                {NP_Name     , (IPTR)"wanderer copy"        },
                {NP_UserData , (IPTR)message_filelist       },
                {NP_StackSize, 40000          },
                {TAG_DONE    , 0                      }
          };

          child = CreateNewProc(tags);
  }
            /* FIXME: update list contents */
        /* this one should be solved through file notofications, as files are copied in a seperate process now  */

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
    struct IconList_Entry *ent;

                NewList(&AppList);

                ent = (void*)MUIV_IconList_NextSelected_Start;
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
                while ( ((int)ent != MUIV_IconList_NextSelected_End) && !fail);
                
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
#ifndef __AROS__
MakeStaticHook(Hook_ActionFunc,Wanderer__HookFunc_ActionFunc);
#endif
///

///Wanderer__HookFunc_StandardFunc()
#ifdef __AROS__
AROS_UFH3
(
    void, Wanderer__HookFunc_StandardFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_StandardFunc, void, void *dummy, void **funcptr)
{
#endif
    AROS_USERFUNC_INIT
    
    void (*func) (ULONG *) = (void (*)(ULONG *)) (*funcptr);
    if (func) func((ULONG *)(funcptr + 1));
    
    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_StandardFunc,Wanderer__HookFunc_StandardFunc);
#endif
///

///Wanderer__HookFunc_BackdropFunc()
#ifdef __AROS__
AROS_UFH3
(
    void, Wanderer__HookFunc_BackdropFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(void *, dummy, A2),
    AROS_UFHA(void **, funcptr, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_BackdropFunc, void, void *dummy, void **funcptr)
{
#endif
    AROS_USERFUNC_INIT

    struct Wanderer_DATA *data = INST_DATA(_WandererIntern_CLASS, _WandererIntern_AppObj);
    BOOL    wb_iscurrentlybd;

D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc()\n"));
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Private data @ %x\n", data));

    if (!data->wd_WorkbenchWindow)
    {
D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: No Workbench Window\n"));
        return;
    }
    
    wb_iscurrentlybd = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_IconWindow_IsBackdrop);

    if (wb_iscurrentlybd != data->wd_Option_BackDropMode)
    {
        BOOL          isOpen = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_Window_Open);
        Object        *win_Active = NULL;

D(bug("[WANDERER] Wanderer__HookFunc_BackdropFunc: Backdrop mode change requested!\n"));
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
#ifndef __AROS__
MakeStaticHook(Hook_BackdropFunc,Wanderer__HookFunc_BackdropFunc);
#endif
///
/******** code from workbench/c/Info.c *******************/
///fmtlarge()
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
///

///GetScreenTitle()
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
///

///GetUserScreenTitle()
STRPTR GetUserScreenTitle(Object *self)
{
  /*Work in progress :-)
   */
    char *screentitlestr;
    int screentitleleng;
    
    GET(self, MUIA_IconWindowExt_ScreenTitle_String, &screentitlestr);   
   
    screentitleleng = strlen(screentitlestr);

    if (screentitleleng<1)
    { 
        return GetScreenTitle();
    } 
    return screentitlestr;

}
///

///ExpandEnvName()
/* Expand a passed in env: string to its full location */
/* Wanderer doesnt free this mem at the moment but should 
   incase it is every closed */
STRPTR ExpandEnvName(STRPTR env_path)
{
    BOOL     ok = FALSE;
    char     tmp_envbuff[1024];
    STRPTR   fullpath = NULL;
    BPTR     env_lock = (BPTR) NULL;

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
///

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
	MEN_ICON_SNAPSHOT,
	MEN_ICON_UNSNAPSHOT,
    MEN_ICON_DELETE,
    MEN_ICON_FORMAT
};


///execute_open_with_command()
/**************************************************************************
Open the execute window. Similar to below but you can also set the
command. Called when item is openend
**************************************************************************/
void execute_open_with_command(BPTR cd, STRPTR contents)
{
    BPTR lock;
    
    if (cd !=(BPTR) NULL) lock =  cd;
    else            lock = Lock("RAM:", ACCESS_READ);
        
    OpenWorkbenchObject
    (
        "WANDERER:Tools/ExecuteCommand",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, (IPTR) contents,
        TAG_DONE
    );
    
    if (cd ==(BPTR) NULL) UnLock(lock);
}
///

///wanderer_menufunc_wanderer_execute()
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
///

///wanderer_menufunc_wanderer_shell()
void wanderer_menufunc_wanderer_shell(STRPTR *cd_ptr)
{
    //TODO: remove the STRPTR *cdptr from top
    //TODO:remove this commented out stuff
    //BPTR cd = Lock(*cd_ptr,ACCESS_READ);
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
    BPTR cd;

    if (!dr)
    {
        dr = "";
    }

    cd = Lock(dr, ACCESS_READ);
    if (SystemTags("NewShell", NP_CurrentDir, (IPTR)cd, TAG_DONE) == -1)
    {
        UnLock(cd);
    }
}
///

///wanderer_menufunc_wanderer_backdrop()
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
///

///wanderer_menufunc_window_newdrawer()
void wanderer_menufunc_window_newdrawer(STRPTR *cdptr)
{
    //TODO: remove the STRPTR *cdptr from top
    
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
    Object *actwindow = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *wbwindow = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_WorkbenchWindow);
    BPTR lock;

D(bug("[wanderer] wanderer_menufunc_window_newdrawer(%s)\n", dr));
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

    lock = Lock(dr, ACCESS_READ);
    OpenWorkbenchObject
    (
        "WANDERER:Tools/WBNewDrawer",
        WBOPENA_ArgLock, (IPTR) lock,
        WBOPENA_ArgName, 0,
        TAG_DONE
    );
    UnLock(lock);
}
///

///wanderer_menufunc_window_openparent()
void wanderer_menufunc_window_openparent(STRPTR *cdptr)
{
    //TODO: Remove the **cdptr stuff from top
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
        
    IPTR  path_len=0;
    STRPTR  last_letter=NULL;
    STRPTR thispath;
    STRPTR buf;
    Object *cstate;
    Object *child;
    BOOL foundSlash, foundColon;
    int i = 0;

    last_letter = &dr[ strlen(dr) - 1 ];
    
    thispath = FilePart(dr);
    
    if (*last_letter==0x3a) return; /* Top Drawer has no parent to open */
    
    last_letter = &thispath[strlen(thispath)-1];
    
    if (*last_letter==0x3a) 
       path_len = (IPTR)(thispath-(IPTR)(dr));
    else 
       path_len = (IPTR)((thispath-(IPTR)(dr))-1);
    
    buf = AllocVec((path_len+1),MEMF_PUBLIC|MEMF_CLEAR);  
    CopyMem(dr, buf, path_len);
    
    cstate = (Object*)(((struct List*)XGET(_WandererIntern_AppObj, MUIA_Application_WindowList))->lh_Head);
   
    
    // Make sure we have a correct path   
    foundSlash = FALSE, foundColon = FALSE;
    i = 0; for ( ; i < path_len; i++ )
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
///

///wanderer_menufunc_window_close()
void wanderer_menufunc_window_close()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    SET(window, MUIA_Window_CloseRequest, TRUE);
}
///

///wanderer_menufunc_window_update()
void wanderer_menufunc_window_update()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_Update);
    }
}
///

///wanderer_menufunc_window_clear()
void wanderer_menufunc_window_clear()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_UnselectAll);
    }
}
///

///wanderer_menufunc_window_select()
void wanderer_menufunc_window_select()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_SelectAll);
    }
}
///

///wanderer_menufunc_window_snapshot()
void wanderer_menufunc_window_snapshot(IPTR *flags)
{
    Object              *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
	Object 				*iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
	char                *dir_info_name = NULL, *icon_info_name = NULL, *dir_name = XGET(window, MUIA_IconWindow_Location);
	IPTR				dir_name_len = strlen(dir_name);
	struct DiskObject 	*drawericon = NULL;
	IPTR				geticon_error = NULL;
	IPTR 				display_bits = 0, sort_bits = 0;
	BOOL 				snapshot_all = *flags;
	BPTR 				tmp_lock;

D(bug("[wanderer] wanderer_menufunc_window_snapshot()\n"));
D(bug("[wanderer] wanderer_menufunc_window_snapshot: Dir '%s'\n", dir_name));

	if ((tmp_lock = Lock(dir_name, ACCESS_WRITE)) != (BPTR)NULL)
	{
		UnLock(tmp_lock);

D(bug("[wanderer] wanderer_menufunc_window_snapshot: Drawer is writable .. continuing ..\n"));

		if (snapshot_all == TRUE)
		{
			struct IconEntry    *entry = NULL;
			struct TagItem  	*icontags = 
			{
				ICONPUTA_OnlyUpdatePosition, TRUE,
				TAG_DONE
			};
D(bug("[wanderer] wanderer_menufunc_window_snapshot: snapshot ALL\n"));

//			if (entry->ile_DiskObj)
//			{
//        			entry->ile_DiskObj->do_CurrentX = entry->ile_IconX;
//        			entry->ile_DiskObj->do_CurrentY = entry->ile_IconY;
//              	    PutIconTagList(icon_info_name, entry->ile_DiskObj, icontags);
//                      }
		}
		else
		{
D(bug("[wanderer] wanderer_menufunc_window_snapshot: snapshot WINDOW\n"));
		}

		if (dir_name[dir_name_len - 1] == ':')
		{
D(bug("[wanderer] %s: Updating Volume ROOT Icon\n", __PRETTY_FUNCTION__));
			if ((dir_info_name = AllocVec(dir_name_len + 10, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
			{
				sprintf(dir_info_name, "%sdisk.info\0", dir_name);
			}
		}
		else
		{
D(bug("[wanderer] %s: Updating Drawer Icon\n", __PRETTY_FUNCTION__));
			if ((dir_info_name = AllocVec(dir_name_len + 6, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
			{
				sprintf(dir_info_name, "%s.info\0", dir_name);
			}
		}
		
		if (dir_info_name)
		{
			if ((tmp_lock = Lock(dir_info_name, SHARED_LOCK)) != (BPTR) NULL)
			{
				UnLock(tmp_lock);
D(bug("[wanderer] wanderer_menufunc_window_snapshot: '%s' found ..\n", dir_info_name));
				drawericon = GetIconTags(dir_info_name,
								ICONGETA_FailIfUnavailable, FALSE,
								ICONA_ErrorCode, &geticon_error,
								TAG_DONE);
			}
			else
			{
				//Get the default ICON ..
D(bug("[wanderer] wanderer_menufunc_window_snapshot: '%s' not found ..using default icon\n", dir_info_name));
				drawericon = GetIconTags(dir_name,
								ICONGETA_FailIfUnavailable, FALSE,
								ICONA_ErrorCode, &geticon_error,
								TAG_DONE);
			}

			if (drawericon != NULL)
			{
				if (drawericon->do_DrawerData == NULL)
				{
D(bug("[wanderer] wanderer_menufunc_window_snapshot: '%s' has no DRAWER data!\n", dir_info_name));
					drawericon->do_DrawerData = AllocMem(sizeof(struct DrawerData), MEMF_CLEAR|MEMF_PUBLIC);
				}

				drawericon->do_Gadget.UserData = 1;

				drawericon->do_DrawerData->dd_NewWindow.TopEdge = XGET(window, MUIA_Window_TopEdge);
				drawericon->do_DrawerData->dd_NewWindow.LeftEdge = XGET(window, MUIA_Window_LeftEdge);
				drawericon->do_DrawerData->dd_NewWindow.Width = XGET(window, MUIA_Window_Width);
				drawericon->do_DrawerData->dd_NewWindow.Height = XGET(window, MUIA_Window_Height);

				GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);
				if (display_bits & ICONLIST_DISP_SHOWINFO)
				{
D(bug("[wanderer] wanderer_menufunc_window_snapshot: ICONLIST_DISP_SHOWINFO\n"));
					drawericon->do_DrawerData->dd_Flags = 1;
				}
				else
				{
					drawericon->do_DrawerData->dd_Flags = 2;
				}

#warning "TODO: Icon sort flags are only really for text list mode ... fix"
				GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
				if (sort_bits & ICONLIST_SORT_BY_DATE)
				{
					drawericon->do_DrawerData->dd_ViewModes = 3;
				}
				else if (sort_bits & ICONLIST_SORT_BY_SIZE)
				{
					drawericon->do_DrawerData->dd_ViewModes = 4;
				}
				else
				{
					drawericon->do_DrawerData->dd_ViewModes = 2;
				}

				PutDiskObject(dir_name, drawericon);
			}
			FreeVec(dir_info_name);
		}
	}
	else
	{
D(bug("[wanderer] wanderer_menufunc_window_snapshot: Drawer is write protected .. skipping ..\n"));
		//DisplayBeep(data->wd_Screen);
		DisplayBeep(NULL);
	}
}
///

///wanderer_menufunc_window_view_icons(Object **pstrip)
void wanderer_menufunc_window_view_icons(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_ALL);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
        IPTR display_bits = 0, menu_view_state = 0;
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
///

///wanderer_menufunc_window_sort_name()
void wanderer_menufunc_window_sort_name(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
        IPTR sort_bits = 0;
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
///

///wanderer_menufunc_window_sort_date()
void wanderer_menufunc_window_sort_date(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
        IPTR sort_bits = 0;
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
///

///wanderer_menufunc_window_sort_size()
void wanderer_menufunc_window_sort_size(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
        IPTR sort_bits = 0;
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
///

///wanderer_menufunc_window_sort_type()
void wanderer_menufunc_window_sort_type(Object **pstrip)
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if ( iconList != NULL)
    {
        IPTR sort_bits = 0;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

        /*type = both date and size bits set*/
        sort_bits |= (ICONLIST_SORT_BY_DATE | ICONLIST_SORT_BY_SIZE);

        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_window_sort_reverse()
void wanderer_menufunc_window_sort_reverse(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_REVERSE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0;
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
///

///wanderer_menufunc_window_sort_topdrawers()
void wanderer_menufunc_window_sort_topdrawers(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_TOPDRAWERS);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0;
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
///

///wanderer_menufunc_icon_open()
void wanderer_menufunc_icon_open()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    DoMethod(window, MUIM_IconWindow_DoubleClicked);
}
///

///wanderer_menufunc_icon_rename()
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
///

///wanderer_menufunc_icon_information()
void wanderer_menufunc_icon_information()
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (IPTR)MUIV_IconList_NextSelected_Start;
    
    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR)&entry);
        
        if ((IPTR)entry != MUIV_IconList_NextSelected_End)
        {
            BPTR lock, parent;
			STRPTR name;

D(bug("[wanderer] wanderer_menufunc_icon_information: selected = '%s'\n", entry->filename));
			lock = Lock(entry->filename, ACCESS_READ);
			name = FilePart(entry->filename);
			if (name[0]) {
				parent = ParentDir(lock);
				UnLock(lock);
			} else
				parent = lock;

			D(bug("[wanderer] wanderer_menufunc_icon_information: name = '%s' lock = 0x%08lX\n", name, lock));
            WBInfo(parent, name, NULL);

            UnLock(parent);
        }
        else
        {
            break;
        }
    } while (TRUE);
}
///

///wanderer_menufunc_icon_snapshot()
void wanderer_menufunc_icon_snapshot(IPTR *flags)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (IPTR)MUIV_IconList_NextSelected_Start;
    struct IconEntry      *node = NULL;
	BOOL 				  snapshot  = *flags;
	struct TagItem  	  *icontags = 
	{
		ICONPUTA_OnlyUpdatePosition, TRUE,
		TAG_DONE
	};

D(bug("[wanderer] wanderer_menufunc_icon_snapshot()\n"));
	if (snapshot)
	{
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: SNAPSHOT'ing\n"));
	}
	else
	{
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: UNSNAPSHOT'ing\n"));
	}

    do
    {
        DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR)&entry);
        
        if ((IPTR)entry != MUIV_IconList_NextSelected_End)
        {
			node = (struct IconEntry *)((IPTR)entry - ((IPTR)&node->ile_IconListEntry - (IPTR)node));
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: entry = '%s' @ %p, (%p)\n", entry->filename, entry, node));
			if (node->ile_DiskObj)
			{
				if (snapshot)
				{
					node->ile_DiskObj->do_CurrentX = node->ile_IconX;
					node->ile_DiskObj->do_CurrentY = node->ile_IconY;
				}
				else
				{
					node->ile_DiskObj->do_CurrentX = NO_ICON_POSITION;
					node->ile_DiskObj->do_CurrentY = NO_ICON_POSITION;
				}
				PutIconTagList(entry->filename, node->ile_DiskObj, icontags);
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: saved ..\n"));
			}
			else
			{
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: icon has no diskobj!\n"));
			}
		}
        else
        {
            break;
        }
    } while (TRUE);
D(bug("[wanderer] wanderer_menufunc_icon_snapshot: finished ..\n"));
}

///DisposeCopyDisplay()
/* dispose the file copy display */
void DisposeCopyDisplay(struct MUIDisplayObjects *d) 
{
    if (d->copyApp) 
    {
        //SET(d->win,MUIA_Window_Open,FALSE);
       MUI_DisposeObject(d->copyApp);
    }
}
///

///CreateCopyDisplay()
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
    d->copyApp = MUI_NewObject(MUIC_Application,
        MUIA_Application_Title,     (IPTR)"CopyRequester",
        MUIA_Application_Base,      (IPTR)"WANDERER_COPY",
        MUIA_Application_Window, (IPTR)(d->win = MUI_NewObject(MUIC_Window,
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
            WindowContents, (group = MUI_NewObject(MUIC_Group,
                Child, (IPTR)(fromObject = MUI_NewObject(MUIC_Text,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                TAG_DONE)),
                Child, (IPTR)(d->sourceObject = MUI_NewObject(MUIC_Text,
                    TextFrame,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                TAG_DONE)),
                Child, (IPTR)(toObject = MUI_NewObject(MUIC_Text,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                TAG_DONE)),
                Child, (IPTR)(d->destObject = MUI_NewObject(MUIC_Text,
                    TextFrame,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                TAG_DONE)),
                Child, (IPTR)(fileTextObject = MUI_NewObject(MUIC_Text,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                TAG_DONE)),
                Child, (IPTR)(d->fileObject = MUI_NewObject(MUIC_Text,
                    TextFrame,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Background,    MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"---",
                TAG_DONE)),
                Child, (IPTR)(fileLengthObject = MUI_NewObject(MUIC_Text,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Text_PreParse, (IPTR)"\33c",
                TAG_DONE)),
                Child, (IPTR)(gaugeGroup = MUI_NewObject(MUIC_Group,
                    TextFrame,
                    Child, d->gauge = MUI_NewObject(MUIC_Gauge,
                        MUIA_Gauge_Horiz, TRUE,
                        MUIA_Gauge_Max, 32768,
                        MUIA_Gauge_InfoText, "Processing...",
                    TAG_DONE),
                    Child, MUI_NewObject(MUIC_Scale,
                        MUIA_Scale_Horiz, TRUE,
                    TAG_DONE),
                TAG_DONE)),
                Child, (IPTR)( d->performanceObject = MUI_NewObject(MUIC_Text,
                    TextFrame,
                    MUIA_InnerLeft,(8),
                    MUIA_InnerRight,(8),
                    MUIA_InnerTop,(2),
                    MUIA_InnerBottom,(2),
                    MUIA_Background,     MUII_TextBack,
                    MUIA_Text_PreParse, (IPTR)"\33c",
                    MUIA_Text_Contents, (IPTR)"...........0 Bytes...........",
                TAG_DONE)),

                Child, (IPTR)( d->stopObject = SimpleButton("Stop") ),
            TAG_DONE)),
        TAG_DONE)),
    TAG_DONE);


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
///

///wanderer_menufunc_icon_delete()
void wanderer_menufunc_icon_delete(void)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = ( void*) MUIV_IconList_NextSelected_Start;
    struct MUIDisplayObjects dobjects;
    struct Hook displayCopyHook;
    struct Hook displayDelHook;
    ULONG updatedIcons;

    DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
    displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
    displayDelHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskDeleteFunc;
    
    updatedIcons = 0;
    
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
///

///wanderer_menufunc_icon_format()
void wanderer_menufunc_icon_format(void)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = ( void*) MUIV_IconList_NextSelected_Start;
    struct MUIDisplayObjects dobjects;
    struct Hook displayCopyHook;
    struct Hook displayDelHook;

    DoMethod(iconList, MUIM_IconList_NextSelected, (IPTR) &entry);
    
    /* Process only first selected entry */
    if ((int)entry != MUIV_IconList_NextSelected_End)
    {  
	BPTR lock   = Lock(entry->filename, ACCESS_READ);
	D(bug("[WANDERER] Format \"%s\"\n", entry->filename);)
	/* Usually we pass object name and parent lock. Here we do the same thing.
	   Just object name is empty string and its parent is device's root. */
        OpenWorkbenchObject
        (
            "SYS:System/Format",
            WBOPENA_ArgLock, (IPTR) lock,
            WBOPENA_ArgName, lock ? (IPTR)"" : (IPTR)entry->filename,
            TAG_DONE
        );
	if (lock)
	    UnLock(lock);
    }
}
///

///wanderer_menufunc_wanderer_guisettings()
void wanderer_menufunc_wanderer_guisettings(void)
{
    //DoMethod(_WandererIntern_AppObj, MUIM_Application_OpenConfigWindow);
    OpenWorkbenchObject("SYS:Prefs/Zune",
                WBOPENA_ArgName, (IPTR) "WANDERER",
                TAG_DONE);
}
///

///wanderer_menufunc_wanderer_about()
void wanderer_menufunc_wanderer_about(void)
{
    OpenWorkbenchObject("SYS:System/About", TAG_DONE);
}
///

///wanderer_menufunc_wanderer_quit()
void wanderer_menufunc_wanderer_quit(void)
{
    if (OpenWorkbenchObject("WANDERER:Tools/Quit", TAG_DONE))
    ;
    else
    {
        if (MUI_RequestA(_WandererIntern_AppObj, NULL, 0, "Wanderer", _(MSG_YESNO), _(MSG_REALLYQUIT), NULL))
        DoMethod(_WandererIntern_AppObj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    }
}
///

///FindMenuitem()
/**************************************************************************
This function returns a Menu Object with the given id
**************************************************************************/
Object *FindMenuitem(Object* strip, int id)
{
    return (Object*)DoMethod(strip, MUIM_FindUData, id);
}
///

///DoMenuNotify()
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
///

///DoAllMenuNotifies()
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

	DoMenuNotify(strip, MEN_WINDOW_SNAP_WIN,        wanderer_menufunc_window_snapshot,        FALSE);
	DoMenuNotify(strip, MEN_WINDOW_SNAP_ALL,        wanderer_menufunc_window_snapshot,        TRUE);
	
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
    DoMenuNotify(strip, MEN_ICON_SNAPSHOT,          wanderer_menufunc_icon_snapshot,          TRUE);
    DoMenuNotify(strip, MEN_ICON_UNSNAPSHOT,        wanderer_menufunc_icon_snapshot,          FALSE);
    DoMenuNotify(strip, MEN_ICON_DELETE,            wanderer_menufunc_icon_delete,            NULL);
    DoMenuNotify(strip, MEN_ICON_FORMAT,	    wanderer_menufunc_icon_format,	      NULL);
    
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
///
/*** Methods ****************************************************************/
///OM_NEW()
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

    //    ULONG updatedIcons;
D(bug("[Wanderer] Wanderer__OM_NEW: SELF = %d, Private data @ %x\n", self, data));

        _WandererIntern_CLASS = CLASS;

#if defined(WANDERER_DEFAULT_BACKDROP)
        data->wd_Option_BackDropMode = TRUE;
#else
        data->wd_Option_BackDropMode = FALSE;
#endif

        /*-- Setup hooks structures ----------------------------------------*/
        #ifdef __AROS__
        _WandererIntern_hook_standard.h_Entry = (HOOKFUNC) Wanderer__HookFunc_StandardFunc;
        _WandererIntern_hook_action.h_Entry   = (HOOKFUNC) Wanderer__HookFunc_ActionFunc;
        _WandererIntern_hook_backdrop.h_Entry   = (HOOKFUNC) Wanderer__HookFunc_BackdropFunc;
        #else
         _WandererIntern_hook_standard = &Hook_StandardFunc;
        _WandererIntern_hook_action   = &Hook_ActionFunc;
        _WandererIntern_hook_backdrop   = &Hook_BackdropFunc;
        #endif

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
        data->wd_PrefsNotifyRequest.nr_Name                 = ExpandEnvName("ENV:SYS/Wanderer.prefs");
        data->wd_PrefsNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
        data->wd_PrefsNotifyRequest.nr_stuff.nr_Msg.nr_Port = data->wd_NotifyPort;

        if (StartNotify(&data->wd_PrefsNotifyRequest))
        {
D(bug("[Wanderer] Wanderer__OM_NEW: Prefs-notification setup on '%s'\n", data->wd_PrefsNotifyRequest.nr_Name));
        }
        else
        {
D(bug("[Wanderer] Wanderer__OM_NEW: FAILED to setup Prefs-notification!\n"));
        }
    #ifdef __AROS__
        data->wd_Prefs = WandererPrefsObject, End; // FIXME: error handling
    #else
    data->wd_Prefs = NewObject(WandererPrefs_CLASS->mcc_Class, NULL, TAG_DONE); // FIXME: error handling
    #endif

D(bug("[Wanderer] Wanderer__OM_NEW: Prefs-SCREENTITLE IS = '%s'\n",XGET(data->wd_Prefs, MUIA_IconWindowExt_ScreenTitle_String)));

        if (data->wd_Prefs) 
            data->wd_PrefsIntern = InitWandererPrefs();
    }
D(bug("[Wanderer] obj = %ld\n", self));
    return self;
}
///

///OM_DISPOSE()
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
    
        EndNotify(&data->wd_PrefsNotifyRequest);
        
        DeleteMsgPort(data->wd_NotifyPort);
        data->wd_NotifyPort = NULL;
        
        DeleteMsgPort(data->wd_CommandPort);
        data->wd_CommandPort = NULL;
        
        DisposeObject(data->wd_Prefs);
        data->wd_Prefs = NULL;
    }
    
    return DoSuperMethodA(CLASS, self, (Msg) message);
}
///

///OM_SET()
IPTR Wanderer__OM_SET(Class *CLASS, Object *self, struct opSet *message)
{
    SETUP_WANDERER_INST_DATA;
    struct TagItem *tstate = message->ops_AttrList, *tag;

    while ((tag = NextTagItem((TAGITEM)&tstate)) != NULL)
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
///

///OM_GET()
IPTR Wanderer__OM_GET(Class *CLASS, Object *self, struct opGet *message)
{
    SETUP_WANDERER_INST_DATA;
    IPTR *store = message->opg_Storage;
    IPTR  rv    = TRUE;

    switch (message->opg_AttrID)
    {
        case MUIA_Wanderer_Screen:
            *store = (IPTR)data->wd_Screen;
            break;

        case MUIA_Wanderer_Prefs:
            *store = (IPTR)data->wd_Prefs;
            break;

        case MUIA_Wanderer_ActiveWindow:
            *store = (IPTR)data->wd_ActiveWindow;
            break;

        case MUIA_Wanderer_WorkbenchWindow:
            *store = (IPTR)data->wd_WorkbenchWindow;
            break;

        case MUIA_Wanderer_FileSysNotifyPort:
            *store = (IPTR)data->wd_NotifyPort;
            break;

        default:
            rv = DoSuperMethodA(CLASS, self, (Msg) message);
    }
    
    return rv;
}
///

///Wanderer__MUIM_Application_Execute()
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

         #ifdef __AROS__
        DoSuperMethodA(CLASS, self, message);
        #else
        {
          IPTR sigs = 0;
          while (DoMethod(self,MUIM_Application_NewInput,&sigs) != MUIV_Application_ReturnID_Quit)
          {
            if (sigs)
            {
              sigs = Wait(sigs | SIGBREAKF_CTRL_C);
              if (sigs & SIGBREAKF_CTRL_C) break;
            }
          }

          return 0;
        }
        #endif
        
        return RETURN_OK;
    }
    
#warning "TODO: Report an error if we fail to create the Workbench's window ..."
    
    return RETURN_ERROR;
}
///

///Wanderer__MUIM_Wanderer_HandleTimer()
/*This function uses GetScreenTitle() function...*/

IPTR Wanderer__MUIM_Wanderer_HandleTimer
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WANDERER_INST_DATA;
    Object *cstate = (Object*)(((struct List*)XGET(self, MUIA_Application_WindowList))->lh_Head);
    Object *child = NULL;

    STRPTR scr_title = GetUserScreenTitle(data->wd_Prefs);

    while ((child = NextObject(&cstate)))
       SET(child, MUIA_Window_ScreenTitle, (IPTR) scr_title);
    
    return TRUE;
}
///

///Wanderer__MUIM_Wanderer_HandleCommand()
IPTR Wanderer__MUIM_Wanderer_HandleCommand
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WANDERER_INST_DATA;
    struct WBHandlerMessage *wbhm = NULL;
    struct List *pub_screen_list;
    struct PubScreenNode *pub_screen_node;
    WORD visitor_count = 0;
D(bug("[Wanderer] %s()\n", __PRETTY_FUNCTION__));
D(bug("[Wanderer] %s: Recieved signal at notification port\n", __PRETTY_FUNCTION__));

    while ((wbhm = WBHM(GetMsg(data->wd_CommandPort))) != NULL)
    {
D(bug("[Wanderer] %s: Recieved message from handler, type = %ld\n", __PRETTY_FUNCTION__, wbhm->wbhm_Type));

        switch (wbhm->wbhm_Type)
        {
            case WBHM_TYPE_SHOW:
D(bug("[Wanderer] %s: WBHM_TYPE_SHOW\n", __PRETTY_FUNCTION__));
                if ((data->wd_Screen = LockPubScreen(NULL)) != NULL)
                {
D(bug("[Wanderer] %s: Unlocking access to screen @ %x\n", __PRETTY_FUNCTION__, data->wd_Screen));
                    UnlockPubScreen(NULL, data->wd_Screen);
                    SET(self, MUIA_ShowMe, TRUE);
                }
                else
                {
#warning "TODO: We need to handle the possiblity that we fail to lock the pubscreen..."
D(bug("[Wanderer] %s: Couldnt Lock WB Screen!!\n", __PRETTY_FUNCTION__));
                }
                break;

            case WBHM_TYPE_HIDE:
D(bug("[Wanderer] %s: WBHM_TYPE_HIDE\n", __PRETTY_FUNCTION__));
                pub_screen_list = LockPubScreenList();

                #ifdef __AROS__
                ForeachNode (pub_screen_list, pub_screen_node)
                #else
    Foreach_Node(pub_screen_list, pub_screen_node);
                #endif
                {
                    if (pub_screen_node->psn_Screen == data->wd_Screen)
                        visitor_count = pub_screen_node->psn_VisitorCount;
                }
                UnlockPubScreenList();
                if (visitor_count == 0)
                    SET(self, MUIA_ShowMe, FALSE);
                break;

            case WBHM_TYPE_UPDATE:
D(bug("[Wanderer] %s: WBHM_TYPE_UPDATE\n", __PRETTY_FUNCTION__));
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

D(bug("[Wanderer] %s: name = %s, length = %ld\n", __PRETTY_FUNCTION__, name, length));

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

D(bug("[Wanderer] %s: Drawer found: %s!\n", __PRETTY_FUNCTION__, child_drawer));

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
D(bug("[Wanderer] %s: WBHM_TYPE_OPEN\n", __PRETTY_FUNCTION__));

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
///

///Wanderer__MUIM_Wanderer_HandleNotify()
IPTR Wanderer__MUIM_Wanderer_HandleNotify
(
    Class *CLASS, Object *self, Msg message
)
{
    SETUP_WANDERER_INST_DATA;
    struct Message *plainMessage = NULL;
    
    while ((plainMessage = GetMsg(data->wd_NotifyPort)) != NULL)
    {
        struct NotifyMessage *notifyMessage = (struct NotifyMessage *) plainMessage;
        IPTR                  notifyMessage_UserData = notifyMessage->nm_NReq->nr_UserData;
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_HandleNotify: got FS notification ('%s' @ 0x%p) userdata = 0x%p!\n", notifyMessage->nm_NReq->nr_Name, notifyMessage, notifyMessage_UserData));

        if ((notifyMessage_UserData ==(IPTR) NULL) && (strcmp(notifyMessage->nm_NReq->nr_Name, data->wd_PrefsNotifyRequest.nr_Name) == 0))
        {
            /* reload prefs file */
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_HandleNotify: Wanderer Prefs-File Changed .. Reloading\n"));

            DoMethod(data->wd_Prefs, MUIM_WandererPrefs_Reload);
        }
        else if (notifyMessage_UserData != (IPTR) NULL)
        {
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_HandleNotify: Drawer Window contents changed .. Updating\n"));
            DoMethod(notifyMessage_UserData, MUIM_IconList_Update);
        }

        ReplyMsg((struct Message *)notifyMessage);
    }

    return 0;
}
///

///__CreateWandererIntuitionMenu__()
/* Some differences here between volumes and subwindows */
Object * __CreateWandererIntuitionMenu__( BOOL isRoot, BOOL isBackdrop)
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
    
            {NM_TITLE,     _(MSG_MEN_WINDOW),  NULL, 0},
    
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
    
        {NM_TITLE,    _(MSG_MEN_ICON),     NULL, 0},
            {NM_ITEM,  _(MSG_MEN_OPEN), _(MSG_MEN_SC_OPEN), 0, 0, (APTR) MEN_ICON_OPEN},
    //    {NM_ITEM,  "Close","C" },
            {NM_ITEM,  _(MSG_MEN_RENAME), _(MSG_MEN_SC_RENAME), 0, 0, (APTR) MEN_ICON_RENAME},
            {NM_ITEM,  _(MSG_MEN_INFO), _(MSG_MEN_SC_INFO), 0, 0, (APTR) MEN_ICON_INFORMATION},
			{NM_ITEM,  "Snapshot", "S", 0, 0, (APTR) MEN_ICON_SNAPSHOT},
			{NM_ITEM,  "Unsnapshot", "U", 0, 0, (APTR) MEN_ICON_UNSNAPSHOT},
    //    {NM_ITEM,  "Leave Out", "L" },
    //    {NM_ITEM,  "Put Away", "P" },
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_DELETE), NULL, 0, 0, (APTR) MEN_ICON_DELETE},
    	    {NM_ITEM,  _(MSG_MEN_FORMAT), NULL, 0, 0, (APTR) MEN_ICON_FORMAT},
    //    {NM_ITEM,  "Empty Trash..." },
    
        {NM_TITLE, _(MSG_MEN_TOOLS),          NULL, 0},
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
    
        {NM_TITLE,     _(MSG_MEN_WINDOW),  NULL, 0},
    
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
    
        {NM_TITLE,    _(MSG_MEN_ICON),     NULL, 0},
            {NM_ITEM,  _(MSG_MEN_OPEN), _(MSG_MEN_SC_OPEN), 0, 0, (APTR) MEN_ICON_OPEN},
    //    {NM_ITEM,  "Close","C" },
            {NM_ITEM,  _(MSG_MEN_RENAME), _(MSG_MEN_SC_RENAME), 0, 0, (APTR) MEN_ICON_RENAME},
            {NM_ITEM,  _(MSG_MEN_INFO), _(MSG_MEN_SC_INFO), 0, 0, (APTR) MEN_ICON_INFORMATION},
			{NM_ITEM,  "Snapshot", "S", 0, 0, (APTR) MEN_ICON_SNAPSHOT},
			{NM_ITEM,  "Unsnapshot", "U", 0, 0, (APTR) MEN_ICON_UNSNAPSHOT},
    //    {NM_ITEM,  "Leave Out", "L" },
    //    {NM_ITEM,  "Put Away", "P" },
            {NM_ITEM, NM_BARLABEL},
            {NM_ITEM,  _(MSG_MEN_DELETE), NULL, 0, 0, (APTR) MEN_ICON_DELETE},
    //    {NM_ITEM,  "Format Disk..." },
    //    {NM_ITEM,  "Empty Trash..." },
    
        {NM_TITLE, _(MSG_MEN_TOOLS),          NULL, 0},
    //    {NM_ITEM,  "ResetWanderer" },
        {NM_END}
        };
        _NewWandIntMenu__menustrip = MUI_MakeObject(MUIO_MenustripNM, nm, (IPTR) NULL);
    }
    return _NewWandIntMenu__menustrip;
}
///

///Wanderer__MUIM_Wanderer_CreateDrawerWindow()
Object *Wanderer__MUIM_Wanderer_CreateDrawerWindow
(
    Class *CLASS, Object *self, 
    struct MUIP_Wanderer_CreateDrawerWindow *message
)
{
    SETUP_WANDERER_INST_DATA;

    Object *window                = NULL;
    BOOL    isWorkbenchWindow     = FALSE;
    BOOL    useBackdrop           = FALSE;
    BOOL    hasToolbar;
    IPTR    TAG_IconWindow_Drawer;
    IPTR    useFont;
    Object *_NewWandDrawerMenu__menustrip;

D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow()\n"));

    if ((isWorkbenchWindow = (message->drawer == NULL ? TRUE : FALSE)))
    {
        useBackdrop = data->wd_Option_BackDropMode;
    }

    hasToolbar            = XGET(data->wd_Prefs, MUIA_IconWindowExt_Toolbar_Enabled);

    TAG_IconWindow_Drawer = isWorkbenchWindow ? TAG_IGNORE : MUIA_IconWindow_Location;

    useFont = (IPTR)NULL;
    
    data->wd_Screen = LockPubScreen(NULL);
    
    if(data->wd_Screen == NULL)
    {
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Couldn't lock screen!\n"));
        CoerceMethod(CLASS, self, OM_DISPOSE);
        return NULL;
    }
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Using Screen @ %x\n", data->wd_Screen));
    
    if (data->wd_PrefsIntern)
    {
        useFont = (IPTR)((struct WandererInternalPrefsData *)data->wd_PrefsIntern)->WIPD_IconFont;
    }

    _NewWandDrawerMenu__menustrip = __CreateWandererIntuitionMenu__ (isWorkbenchWindow, useBackdrop);

//D(bug("1\n\n")); Delay(100);
    /* Create a new icon drawer window with the correct drawer being set */
        
    #ifdef __AROS__ 
    window = IconWindowObject,
                MUIA_UserData,                      1,
                MUIA_Wanderer_Prefs,                  data->wd_Prefs,
                MUIA_Wanderer_Screen,                 data->wd_Screen,
                MUIA_Window_ScreenTitle,              GetUserScreenTitle(data->wd_Prefs),
                MUIA_Window_Menustrip,                (IPTR) _NewWandDrawerMenu__menustrip,
                TAG_IconWindow_Drawer,                (IPTR) message->drawer,
                MUIA_IconWindow_Font,                 useFont,
                MUIA_IconWindow_ActionHook,           (IPTR) &_WandererIntern_hook_action,
                MUIA_IconWindow_IsRoot,               isWorkbenchWindow ? TRUE : FALSE,
                isWorkbenchWindow ? MUIA_IconWindow_IsBackdrop : TAG_IGNORE, useBackdrop,
                isWorkbenchWindow ? TAG_IGNORE : MUIA_Wanderer_FileSysNotifyPort, data->wd_NotifyPort,
                MUIA_Window_IsSubWindow,              isWorkbenchWindow ? FALSE : TRUE,
                MUIA_IconWindowExt_Toolbar_Enabled,   hasToolbar ? TRUE : FALSE,
             End;
    #else
     window = NewObject(IconWindow_CLASS->mcc_Class, NULL,
                MUIA_UserData,                      1,
                MUIA_Wanderer_Prefs,                  data->wd_Prefs,
                MUIA_Wanderer_Screen,                 data->wd_Screen,
                MUIA_Window_ScreenTitle,              GetUserScreenTitle(data->wd_Prefs),
                MUIA_Window_Menustrip,                (IPTR) _NewWandDrawerMenu__menustrip,
                TAG_IconWindow_Drawer,                (IPTR) message->drawer,
                MUIA_IconWindow_Font,                 useFont,
                MUIA_IconWindow_ActionHook,           (IPTR) &_WandererIntern_hook_action,
                MUIA_IconWindow_IsRoot,               isWorkbenchWindow ? TRUE : FALSE,
                isWorkbenchWindow ? MUIA_IconWindow_IsBackdrop : TAG_IGNORE, useBackdrop,
                isWorkbenchWindow ? TAG_IGNORE : MUIA_Wanderer_FileSysNotifyPort, data->wd_NotifyPort,
                MUIA_Window_IsSubWindow,              isWorkbenchWindow ? FALSE : TRUE,
                MUIA_IconWindowExt_Toolbar_Enabled,   hasToolbar ? TRUE : FALSE,
             TAG_DONE);
    #endif
//D(bug("2\n\n")); Delay(100);
    if (data->wd_Screen)
    {
D(bug("[Wanderer] Wanderer__MUIM_Wanderer_CreateDrawerWindow: Unlocking access to screen @ %x\n", data->wd_Screen));
        UnlockPubScreen(NULL, data->wd_Screen);
    }

    if (window != NULL)
    {
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: window != NULL\n"));
        /* Get the drawer path back so we can use it also outside this function */
        STRPTR drw = NULL;
        BOOL freeDrwStr = FALSE;
        
        if (!isWorkbenchWindow) drw = (STRPTR) XGET(window, MUIA_IconWindow_Location);
        else
        {
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: call AllocVec()\n"));
            drw = AllocVec ( 5, MEMF_CLEAR );
            sprintf ( drw, "RAM:" );
            freeDrwStr = TRUE;    
        }
        
        if (isWorkbenchWindow)
        {
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: isWorkbenchWindow\n"));
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
                (IPTR)_app(self), 4, MUIM_Application_PushMethod, (IPTR)window, 1, MUIM_IconWindow_Remove
            );
        }

#if defined(WANDERER_DEFAULT_SHOWALL) || defined(WANDERER_DEFAULT_SHOWHIDDEN)
        Object *window_IconList = NULL;
        IPTR  current_DispFlags = 0;
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: call get with MUIA_IconWindow_IconList\n"));
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
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: setup notifications\n"));
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
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: execute all notifies\n"));
        /* If "Execute Command" entry is clicked open the execute window */
        DoAllMenuNotifies(_NewWandDrawerMenu__menustrip, drw);        

D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: add window to app\n"));
        /* Add the window to the application */
        #ifdef __AROS__
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) window);
        #else
        DoMethod(self, OM_ADDMEMBER, (IPTR) window);
        #endif
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: open window\n"));
        /* And now open it */
        DoMethod(window, MUIM_IconWindow_Open);
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: clean up memory\n"));
        /* Clean up ram string */
        if ( freeDrwStr && drw ) FreeVec ( drw );
    }
D(bug("Wanderer__MUIM_Wanderer_CreateDrawerWindow: exit\n"));
    return window;
}
///
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
