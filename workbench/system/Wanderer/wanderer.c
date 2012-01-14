/*
    Copyright  2004-2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#define ZCC_QUIET

#include "portable_macros.h"

#ifdef __AROS__
#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>
#endif

#define WANDERER_DEFAULT_BACKDROP

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

#include <zune/iconimage.h>

#include "iconwindow.h"
#include "iconwindow_attributes.h"
#include "iconwindow_iconlist.h"
#include "wandererprefs.h"
#include "filesystems.h"
#include "wanderer.h"
#include "Classes/iconlist.h"
#include "Classes/iconlist_attributes.h"
#include "locale.h"
#include "appobjects.h"

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

Object                  *FindMenuitem(Object* strip, int id);
Object                  *Wanderer__Func_CreateWandererIntuitionMenu(BOOL isRoot, BOOL useBackdrop);
void                    wanderer_menufunc_window_update(void);
void                    wanderer_menufunc_window_cleanup(void);
void                    execute_open_with_command(BPTR cd, STRPTR contents);
void                    DisposeCopyDisplay(struct MUIDisplayObjects *d);
BOOL                    CreateCopyDisplay(UWORD flags, struct MUIDisplayObjects *d);

static struct List      _WandererIntern_FSHandlerList;

/* Stored in the main wanderer executable */
extern Object           *_WandererIntern_AppObj;
extern Class            *_WandererIntern_CLASS;
/* Internal Hooks */
#ifdef __AROS__
struct Hook             _WandererIntern_hook_standard;
struct Hook             _WandererIntern_hook_action;
struct Hook             _WandererIntern_hook_backdrop;
#else
struct Hook             *_WandererIntern_hook_standard;
struct Hook             *_WandererIntern_hook_action;
struct Hook             *_WandererIntern_hook_backdrop;
#endif

/*** Instance Data **********************************************************/
struct Wanderer_DATA
{
    struct Screen                       *wd_Screen;

    Object                              *wd_Prefs,
                                        *wd_ActiveWindow,
                                        *wd_WorkbenchWindow,
                                        *wd_AboutWindow;

    struct MUI_InputHandlerNode         wd_TimerIHN;
    struct MsgPort                      *wd_CommandPort;
    struct MUI_InputHandlerNode         wd_CommandIHN;
    struct MsgPort                      *wd_NotifyPort;
    struct MUI_InputHandlerNode         wd_NotifyIHN;

    BOOL                                wd_Option_BackDropMode;
};

const UBYTE     wand_titlestr[] = WANDERERSTR;
const UBYTE     wand_versionstr[] = VERSION;
const UBYTE     wand_copyrightstr[] = WANDERERCOPY;
const UBYTE     wand_authorstr[] = WANDERERAUTH;
const UBYTE     wand_namestr[] = WANDERERNAME;

const UBYTE     wand_copyprocnamestr[] = WANDERERNAME" FileCopy Operation";

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

        if (d->smallobjects > 0)
            d->updateme = FALSE;

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
                if (rate < 1024.0) sprintf(d->SpeedBuffer, "%.2f kBytes/s",  rate); else sprintf(d->SpeedBuffer, "%.2f MBytes/s",  rate / 1024.0);
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
                        d->Buffer, "%s %ld   %s %.2f kBytes   %s %.2f kBytes", 
                        _(MSG_WANDERER_FILEACCESS_NOOFFILES), (long)d->numfiles, _(MSG_WANDERER_FILEACCESS_ACTUAL), (double) obj->filelen / 1024.0, _(MSG_WANDERER_FILEACCESS_TOTAL), (double) d->bytes / 1024.0
                    );
                }
                else
                {
                    sprintf(
                        d->Buffer, "%s %ld   %s %.2f MBytes   %s %.2f kBytes", 
                        _(MSG_WANDERER_FILEACCESS_NOOFFILES), (long)d->numfiles, _(MSG_WANDERER_FILEACCESS_ACTUAL), (double) obj->filelen / 1048576.0, _(MSG_WANDERER_FILEACCESS_TOTAL), (double) d->bytes / 1024.0
                    );
                }
            }
            else
            {
                if (obj->filelen < 1048576)
                {
                    sprintf(
                        d->Buffer, "%s %ld   %s %.2f kBytes   %s %.2f MBytes", 
                        _(MSG_WANDERER_FILEACCESS_NOOFFILES), (long)d->numfiles, _(MSG_WANDERER_FILEACCESS_ACTUAL), (double) obj->filelen / 1024.0, _(MSG_WANDERER_FILEACCESS_TOTAL), (double) d->bytes / 1048576.0
                    );
                }
                else
                {
                    sprintf(
                        d->Buffer, "%s %ld   %s %.2f MBytes   %s %.2f MBytes", 
                        _(MSG_WANDERER_FILEACCESS_NOOFFILES), (long)d->numfiles, _(MSG_WANDERER_FILEACCESS_ACTUAL), (double) obj->filelen / 1048576.0, _(MSG_WANDERER_FILEACCESS_TOTAL), (double) d->bytes / 1048576.0
                    );
                }
            }
            SET(d->performanceObject, MUIA_Text_Contents, d->Buffer);
        }
    }

    DoMethod(d->copyApp, MUIM_Application_InputBuffered);

    /* read the stopflag and return TRUE if the user wanted to stop actionDir() */
    if (d->stopflag == 1)
        return TRUE;
    else
        return FALSE;

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_DisplayCopyFunc,Wanderer__HookFunc_DisplayCopyFunc);
#endif
///

///Wanderer__HookFunc_AskModeFunc()
#ifdef __AROS__
AROS_UFH3
(
    ULONG, Wanderer__HookFunc_AskModeFunc,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(struct dCopyStruct *, obj, A2),
    AROS_UFHA(APTR, unused_param, A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_AskModeFunc, ULONG, struct dCopyStruct *obj, APTR unused_param)
{
#endif
    AROS_USERFUNC_INIT

    ULONG back = OPMODE_NONE;

    UWORD    ret = 0;
    char     *string = NULL;

    if (obj->file) 
    {
        if (obj->type == 0) 
        {
            string = CombineString("%s\n\033b%s\033n\n%s\n\033b%s\033n %s", 
                _(MSG_REQU_DELETE_FILE_S), obj->file, _(MSG_REQU_DELETE_FILE_M), obj->spath, _(MSG_REQU_DELETE_FILE_E) );
        }
        else if (obj->type == 1) 
        {
            string = CombineString("%s\n\033b%s\033n\n%s\n\033b%s\033n %s", 
                _(MSG_REQU_FILEUNPROTECT_S), obj->file, _(MSG_REQU_FILEUNPROTECT_M), obj->spath, _(MSG_REQU_FILEUNPROTECT_E) );
        }
        else if (obj->type == 2) 
        {
            string = CombineString("%s\n\033b%s\033n\n%s\n\033b%s\033n %s", 
                _(MSG_REQU_OVERWRITE_S), obj->file, _(MSG_REQU_OVERWRITE_M), obj->spath, _(MSG_REQU_OVERWRITE_E) );
        }
        else 
        {
            string = CombineString("%s\n\033b%s\033n\n%s\n\033b%s\033n%s", 
                _(MSG_NOFILEACCESS_S), obj->file, _(MSG_NOFILEACCESS_M), obj->spath, _(MSG_NOFILEACCESS_E) );
        }
    } 
    else 
    {
        if (obj->type == 0) string = CombineString("%s \033b%s\033n %s", _(MSG_REQU_DELETE_DRAWER_S), obj->spath, _(MSG_REQU_DELETE_DRAWER_E) );
        else if (obj->type == 1) string = CombineString("%s\n\033b%s\033n %s", _(MSG_REQU_PROTECTION_UNPROTECTDRAWER_S), obj->spath, _(MSG_REQU_PROTECTION_UNPROTECTDRAWER_E) );
        else if (obj->type == 3) string = CombineString("%s\n\033b%s %s", _(MSG_NODRAWERACCESS_S), obj->spath, _(MSG_NODRAWERACCESS_E) );
    }

    if (string) 
    {
        if (obj->type == 0) ret = AskChoiceCentered( _(MSG_REQU_DELETE), string, _(MSG_REQU_DELETE_YESNO), 0);
        else if (obj->type == 1) ret = AskChoiceCentered( _(MSG_REQU_PROTECTION), string, _(MSG_REQU_PROTECTION_UNPROTECT), 0);
        else if (obj->type == 2) ret = AskChoiceCentered( _(MSG_REQU_OVERWRITE), string, _(MSG_REQU_OVERWRITE_YESNO), 0);
        else ret = AskChoiceCentered( _(MSG_REQU_OVERWRITE), string, _(MSG_REQU_OVERWRITE_SKIPABORT), 0);
        freeString(NULL, string);
    }

    if (ret == 0) back = OPMODE_NONE;
    else if (ret == 1) back = OPMODE_YES;
    else if (ret == 2) back = OPMODE_ALL;
    else if (ret == 3) back = OPMODE_NO;

    return back;

    AROS_USERFUNC_EXIT
}
#ifndef __AROS__
MakeStaticHook(Hook_AskModeFunc,Wanderer__HookFunc_AskModeFunc);
#endif
///

///Wanderer__Func_CopyDropEntries()
AROS_UFH3(void, Wanderer__Func_CopyDropEntries,
        AROS_UFHA(STRPTR,              argPtr, A0),
        AROS_UFHA(ULONG,               argSize, D0),
        AROS_UFHA(struct ExecBase *,   SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct IconList_Drop_Event *copyFunc_DropEvent = FindTask(NULL)->tc_UserData;

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (copyFunc_DropEvent)
    {
        struct MUIDisplayObjects dobjects;
        struct IconList_Drop_SourceEntry *currententry;
        struct OpModes opModes;
        ULONG updatedIcons = 0;

        opModes.deletemode = OPMODE_ASK;
        opModes.protectmode = OPMODE_ASK;
        opModes.overwritemode = OPMODE_ASK;
#ifdef __AROS__
        struct Hook displayCopyHook;
        struct Hook displayAskHook;
        displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
        displayAskHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskModeFunc;
        opModes.askhook = &displayAskHook;
#else
        struct Hook *displayCopyHook;
        struct Hook *displayAskHook;
        displayCopyHook = &Hook_DisplayCopyFunc;
        displayAskHook = &Hook_AskModeFunc;
        opModes.askhook = displayAskHook;
#endif

        if (CreateCopyDisplay(ACTION_COPY, &dobjects))
        {
            while ((currententry = (struct IconList_Drop_SourceEntry *)RemTail(&copyFunc_DropEvent->drop_SourceList)) != NULL)
            {
                D(bug("[Wanderer] %s: Copying '%s' to '%s'\n", __PRETTY_FUNCTION__,
                        currententry->dropse_Node.ln_Name, copyFunc_DropEvent->drop_TargetPath));

                CopyContent(NULL,
                            currententry->dropse_Node.ln_Name, copyFunc_DropEvent->drop_TargetPath,
                            TRUE, ACTION_COPY, &displayCopyHook, &opModes, (APTR) &dobjects);
                updatedIcons++;

                FreeVec(currententry->dropse_Node.ln_Name);
                FreeMem(currententry, sizeof(struct IconList_Drop_SourceEntry));
            } 
            /* delete copy window */
            DisposeCopyDisplay(&dobjects);
        }

        if (updatedIcons > 0)
        {
            /* Update state of target object after copying */
            DoMethod(_app(copyFunc_DropEvent->drop_TargetObj), MUIM_Application_PushMethod,
                    copyFunc_DropEvent->drop_TargetObj, 1, MUIM_IconList_Update);
            DoMethod(_app(copyFunc_DropEvent->drop_TargetObj), MUIM_Application_PushMethod,
                    copyFunc_DropEvent->drop_TargetObj, 1, MUIM_IconList_Sort);
        }

        if (copyFunc_DropEvent->drop_TargetPath) FreeVec(copyFunc_DropEvent->drop_TargetPath);
        FreeMem(copyFunc_DropEvent, sizeof(struct IconList_Drop_Event));
    }
    return;

    AROS_USERFUNC_EXIT
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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (msg->type == ICONWINDOW_ACTION_OPEN)
    {
        static unsigned char  buf[1024];
        D(IPTR                  offset);
        struct IconList_Entry *ent = (void*)MUIV_IconList_NextIcon_Start;

        DoMethod(msg->iconlist, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&ent);
        if ((IPTR)ent == (IPTR)MUIV_IconList_NextIcon_End)
        {
D(bug("[Wanderer] %s: ICONWINDOW_ACTION_OPEN: NextIcon returned MUIV_IconList_NextIcon_TAG_DONE)\n", __PRETTY_FUNCTION__));
            return;
        }

        D(offset = strlen(ent->ile_IconEntry->ie_IconNode.ln_Name) - 5);

        if ((msg->isroot) && (ent->type == ST_ROOT))
        {
            strcpy((STRPTR)buf, ent->label);
        }
        else
        {
            strcpy((STRPTR)buf, ent->ile_IconEntry->ie_IconNode.ln_Name);
        }

D(bug("[Wanderer] %s: ICONWINDOW_ACTION_OPEN: offset = %d, buf = %s\n", __PRETTY_FUNCTION__, offset, buf));

        if  ((ent->type == ST_ROOT) || (ent->type == ST_USERDIR) || (ent->type == ST_LINKDIR))
        {
            Object *cstate = (Object*)(((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
            Object *prefs = (Object*) XGET(_app(obj), MUIA_Wanderer_Prefs);
            Object *child;

            /* open new window if root or classic navigation set */
            if ( (msg->isroot) || (XGET(prefs, MUIA_IconWindow_WindowNavigationMethod) == WPD_NAVIGATION_CLASSIC) )
            {
                /* Check if the window for this drawer is already opened */
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
                DoMethod(_app(obj), MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf);
                // FIXME: error handling
            }
            else
            {
                /* Open drawer in same window */
                SET(obj, MUIA_IconWindow_Location, (IPTR) buf);
            }
        } 
        else if ((ent->type == ST_FILE) || (ent->type == ST_LINKFILE))
        {
            BPTR newwd, oldwd, file;

            /* Set the CurrentDir to the path of the executable to be started */
            file = Lock(ent->ile_IconEntry->ie_IconNode.ln_Name, SHARED_LOCK);
            if(file)
            {
                newwd = ParentDir(file);
                oldwd = CurrentDir(newwd);
                struct IconList_Entry *ent2        = (void*)MUIV_IconList_NextIcon_Start;
                int                    argsCounted = 0,
                                       i           = 0;
                struct TagItem        *argsTagList = NULL;
                Object                *firstWindow = (Object *) (((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
                Object                *windowItem,
                                      *iconList;

                /*
                ** If we have more than one icon selected, the first one
                ** is our command,  and the next ones are the arguments.
                ** We take care of icons selected on other windows.
                */
                
                /* Count the selected icons */
                while ( (windowItem = NextObject(&firstWindow)) )
                {
                    iconList = (Object *) XGET(windowItem, MUIA_IconWindow_IconList);
                    if (iconList != NULL) /* Wanderer has non-iconlist windows as well */
                    {
                        ent2     = (void*) MUIV_IconList_NextIcon_Start;
                        DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&ent2);
                        do
                        {
                            if ((IPTR)ent2 != MUIV_IconList_NextIcon_End )
                                argsCounted++;
                            DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&ent2);
                        }
                        while ((IPTR)ent2 != MUIV_IconList_NextIcon_End );
                    }
                } /* while ( (windowItem = NextObject(&firstWindow)) ) */
                D(bug("[Wanderer] argsCounted = %d\n", argsCounted));
                        
                /* If we have arguments, populate argsTagList with them */
                if ( argsCounted > 1 ) /* "ent" is selected and has been counted */
                {
                    argsTagList = AllocateTagItems(argsCounted);
                    firstWindow = (Object *) (((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
                    while ( (windowItem = NextObject(&firstWindow)) )
                    {
                        iconList = (Object *) XGET(windowItem, MUIA_IconWindow_IconList);
                        if (iconList != NULL) /* Wanderer has non-iconlist windows as well */
                        {
                            ent2     = (void*) MUIV_IconList_NextIcon_Start;
                            DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&ent2);

                            while ((IPTR)ent2 != MUIV_IconList_NextIcon_End )
                            {
                                if ( ent2->ile_IconEntry->ie_IconNode.ln_Name != ent->ile_IconEntry->ie_IconNode.ln_Name )
                                {
                                    argsTagList[i].ti_Tag  = WBOPENA_ArgName;
                                    argsTagList[i].ti_Data = (IPTR) ent2->ile_IconEntry->ie_IconNode.ln_Name;
                                    D(bug("[Wanderer] argsTagList[%d]: %s\n", i, argsTagList[i].ti_Data));
                                    i++;
                                }
                                DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&ent2);
                            }
                        }
                    } /* while ( (windowItem = NextObject(&cstate)) ) */
                    argsTagList[(argsCounted - 1)].ti_Tag = TAG_DONE;
                    /*
                    ** TODO: the user should be able to select the tool in one window, and some arguments
                    ** in other windows, in that very order. For now Wanderer only handles selected icons
                    ** per window, and so doesn't provide an easy way to know about _all_ selected icons.
                    ** At least we can browse all windows and keep track of all selected icons on each of
                    ** them. But then we don't have a way to know which icon was selected first (which is
                    ** the tool one). The trick for the user is to select first all arguments that aren't
                    ** in  the  tool's window, and only then select the tool on its window and  the  last
                    ** arguments in the tool's window, and of course double-click the very last icon.
                    */
                } /* if (argsCounted > 1) */

                if ( !OpenWorkbenchObjectA(ent->ile_IconEntry->ie_IconNode.ln_Name, argsTagList) )
                {
                    execute_open_with_command(newwd, FilePart(ent->ile_IconEntry->ie_IconNode.ln_Name));
                }
                FreeTagItems(argsTagList); /* FreeTagItems() only frees memory if non NULL */
                CurrentDir(oldwd);
                UnLock(newwd);
                UnLock(file);
            }
        }
        else if (ent->type == ILE_TYPE_APPICON)
        {
            SendAppIconMessage((struct AppIcon *)ent->ile_IconEntry->ie_AppIcon, 0, NULL);
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
            Object *cstate = (Object*)(((struct List*)XGET(_app(obj), MUIA_Application_WindowList))->lh_Head);
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
        struct Process                  *wandererCopyProcess;
        struct IconList_Drop_Event      *dropevent = (struct IconList_Drop_Event *)msg->drop;

        {
            wandererCopyProcess = CreateNewProcTags(
                                NP_Entry,       (IPTR)Wanderer__Func_CopyDropEntries,
                                NP_Name,        (IPTR)wand_copyprocnamestr,
                                NP_Synchronous, FALSE,
                                NP_UserData,    (IPTR)dropevent,
                                NP_StackSize,   40000,
                                TAG_DONE);
            if (wandererCopyProcess == NULL) {
                /* TODO: Handle failure to create the copy process */
            }
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

                ent = (void*)MUIV_IconList_NextIcon_Start;
                /* process all selected entries */
                do 
                {
                    DoMethod(msg->iconlist, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR) &ent);
                    /*  if not end of selection, process */
                    if ((IPTR)ent != MUIV_IconList_NextIcon_End )
                    {
                        struct AppW *a = AllocVec(sizeof(struct AppW), MEMF_CLEAR);
                        if (a)
                        {
                            a->name = AllocVec(strlen(ent->ile_IconEntry->ie_IconNode.ln_Name)+2, MEMF_CLEAR);
                            if (a->name)
                            {
                                files++;
                                strcpy(a->name, ent->ile_IconEntry->ie_IconNode.ln_Name);
                                if((ent->type == ST_LINKDIR) || (ent->type == ST_USERDIR))
                                {
                                    D(bug("[Wanderer] %s: ent->type=%d\n", __PRETTY_FUNCTION__, ent->type));
                                    strcat (a->name, "/");
                                }
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
                while (((IPTR)ent != MUIV_IconList_NextIcon_End) && !fail);

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

D(bug("[Wanderer] %s: win:<%s> first file:<%s> mx=%d my=%d\n", __PRETTY_FUNCTION__,
                                    win->Title, *filelist,
                                    wscreen->MouseX - win->LeftEdge, wscreen->MouseY - win->TopEdge);)

                            /* send appwindow msg struct containing selected files to destination */
                            SendAppWindowMessage(win, files, (char **)filelist, 0, wscreen->MouseX - win->LeftEdge, wscreen->MouseY - win->TopEdge, 0, 0);

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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

D(bug("[Wanderer] %s: Private data @ %x\n", __PRETTY_FUNCTION__, data));

    if (!data->wd_WorkbenchWindow)
    {
D(bug("[Wanderer] %s: No Workbench Window\n", __PRETTY_FUNCTION__));
        return;
    }

    wb_iscurrentlybd = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_IconWindow_IsBackdrop);

    if (wb_iscurrentlybd != data->wd_Option_BackDropMode)
    {
        BOOL          isOpen = (BOOL)XGET(data->wd_WorkbenchWindow, MUIA_Window_Open);
        Object        *win_Active = NULL;

D(bug("[Wanderer] %s: Backdrop mode change requested!\n", __PRETTY_FUNCTION__));
D(bug("[Wanderer] %s: Disposing of existing Workbench window Obj ..\n", __PRETTY_FUNCTION__));
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
D(bug("[Wanderer] %s: Creating new Workbench window Obj (BACKDROP MODE)..\n", __PRETTY_FUNCTION__));
        }
        else
        {
D(bug("[Wanderer] %s: Creating new Workbench window Obj (NORMAL MODE)..\n", __PRETTY_FUNCTION__));
        }
#endif
        data->wd_WorkbenchWindow = (Object *) DoMethod
          (
            _WandererIntern_AppObj, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
          );

        if ((data->wd_WorkbenchWindow) && (isOpen))
        {
D(bug("[Wanderer] %s: Making Workbench window visable..\n", __PRETTY_FUNCTION__));
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
    const UBYTE *ch;
    struct
    {
        IPTR val;
        IPTR  dec;
    } array =
    {
        num,
        0
    };

    if (num >= 0x40000000)
    {
        array.val = num >> 30;
        d = ((UQUAD)num * 10 + 0x20000000) / 0x40000000;
        array.dec = d % 10;
        //ch = 'G';
        ch = _(MSG_MEM_G);
    }
    else if (num >= 0x100000)
    {
        array.val = num >> 20;
        d = ((UQUAD)num * 10 + 0x80000) / 0x100000;
        array.dec = d % 10;
        //ch = 'M';
        ch = _(MSG_MEM_M);
    }
    else if (num >= 0x400)
    {
        array.val = num >> 10;
        d = (num * 10 + 0x200) / 0x400;
        array.dec = d % 10;
        //ch = 'K';
        ch = _(MSG_MEM_K);
    }
    else
    {
        array.val = num;
        array.dec = 0;
        d = 0;
        //ch = 'B';
                ch = _(MSG_MEM_B);
    }

    if (!array.dec && (d > array.val * 10))
    {
        array.val++;
    }

    RawDoFmt(array.dec ? "%lu.%lu" : "%lu", &array, NULL, buf);
    while (*buf) { buf++; }
    *buf++ = *ch;
    *buf   = '\0';
}
///

///GetScreenTitle()
STRPTR GetScreenTitle(VOID)
{
    static TEXT         title[256];
    UBYTE               chip[10], fast[10];
    ULONG               __availMem;

    __availMem = AvailMem(MEMF_CHIP);
    fmtlarge(chip, __availMem);
    __availMem = AvailMem(MEMF_FAST);
    fmtlarge(fast, __availMem);

    sprintf(title, _(MSG_SCREENTITLE), chip, fast);

    return title;
}
///

///GetUserScreenTitle()
STRPTR GetUserScreenTitle(Object *self)
{
  /*Work in progress :-)
   */
    char *screentitlestr = NULL;
    int screentitleleng;

    GET(self, MUIA_IconWindowExt_ScreenTitle_String, &screentitlestr);   

    if (screentitlestr == NULL)
        screentitlestr = "";

    screentitleleng = strlen(screentitlestr);

    if (screentitleleng<1)
    { 
        return GetScreenTitle();
    } 
    return screentitlestr;

}
///

enum
{
    MEN_WANDERER = 1,
    MEN_WANDERER_BACKDROP,
    MEN_WANDERER_EXECUTE,
    MEN_WANDERER_SHELL,
    MEN_WANDERER_AROS_GUISETTINGS,
    MEN_WANDERER_AROS_ABOUT,
    MEN_WANDERER_ABOUT,
    MEN_WANDERER_QUIT,
    MEN_WANDERER_SHUTDOWN,

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
    MEN_WINDOW_VIEW_HIDDEN,

    MEN_WINDOW_SORT_ENABLE,
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
    MEN_ICON_LEAVEOUT,
    MEN_ICON_PUTAWAY,
    MEN_ICON_DELETE,
    MEN_ICON_FORMAT,
    MEN_ICON_EMPTYTRASH
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
    Object *win = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    STRPTR dr = ( STRPTR )XGET( win, MUIA_IconWindow_Location );
    BPTR cd, olddir;

    if (!dr)
    {
        dr = "";
    }

    cd = Lock(dr, ACCESS_READ);
    olddir = CurrentDir(cd);
    Execute("", BNULL, BNULL);
    CurrentDir(olddir);
    UnLock(cd);
}
///

///wanderer_menufunc_wanderer_backdrop()
void wanderer_menufunc_wanderer_backdrop(Object **pstrip)
{
    struct Wanderer_DATA *data = INST_DATA(_WandererIntern_CLASS, _WandererIntern_AppObj);
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WANDERER_BACKDROP);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

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

D(bug("[Wanderer]: %s('%s')\n", __PRETTY_FUNCTION__, dr));

    if (actwindow == wbwindow)
    {
        /* This check is necessary because WorkbenchWindow has path RAM: */
D(bug("[Wanderer] %s: Can't call WBNewDrawer for WorkbenchWindow\n", __PRETTY_FUNCTION__));
        return;
    }
    if ( XGET(actwindow, MUIA_Window_Open) == FALSE )
    {
D(bug("[Wanderer] %s: Can't call WBNewDrawer: the active window isn't open\n", __PRETTY_FUNCTION__));
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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    SET(window, MUIA_Window_CloseRequest, TRUE);
}
///

///wanderer_menufunc_window_update()
void wanderer_menufunc_window_update()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextIcon_Start;

    D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (iconList != NULL)
    {
        /*
         * Reset the ie_Provided[X|Y] to DiskObject values. This will cause all icons to return to their
         * snapshoted position or get layouted. This is bahavior only present in this menu action. This is Wanderer
         * function, not IconList function, thus the implementation is here.
         */
        do
        {
            DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Visible, (IPTR) &entry);

            if ((IPTR)entry != MUIV_IconList_NextIcon_End)
            {
                if (entry->ile_IconEntry->ie_DiskObj)
                {
                    entry->ile_IconEntry->ie_ProvidedIconX = entry->ile_IconEntry->ie_DiskObj->do_CurrentX;
                    entry->ile_IconEntry->ie_ProvidedIconY = entry->ile_IconEntry->ie_DiskObj->do_CurrentY;
                }
                else
                {
                    entry->ile_IconEntry->ie_ProvidedIconX = NO_ICON_POSITION;
                    entry->ile_IconEntry->ie_ProvidedIconY = NO_ICON_POSITION;
                }
            }
            else break;
        } while (TRUE);

        DoMethod(iconList, MUIM_IconList_Update);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_window_cleanup()
void wanderer_menufunc_window_cleanup()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (iconList != NULL)
    {
        /*
         * Cleanup ignores all snapshoted / provided positions and re-layouts all icons. To enable this,
         * temporarilly set the AutoSort flag. This is Wanderer function, not IconList function, thus the
         * implementation is here.
         */
        IPTR sortFlags;

        get(iconList, MUIA_IconList_SortFlags, &sortFlags);

        if ((sortFlags & MUIV_IconList_Sort_AutoSort) == 0)
        {
            sortFlags |= MUIV_IconList_Sort_AutoSort;
            set(iconList, MUIA_IconList_SortFlags, sortFlags);

            DoMethod(iconList, MUIM_IconList_Sort);

            get(iconList, MUIA_IconList_SortFlags, &sortFlags);
            sortFlags &= ~MUIV_IconList_Sort_AutoSort;
            set(iconList, MUIA_IconList_SortFlags, sortFlags);
        }
        else
        {
            DoMethod(iconList, MUIM_IconList_Sort);
        }
    } 
}
///



///wanderer_menufunc_window_clear()
void wanderer_menufunc_window_clear()
{
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (iconList != NULL)
    {
        DoMethod(iconList, MUIM_IconList_SelectAll);
    }
}
///

///wanderer_menufunc_window_snapshot()
void wanderer_menufunc_window_snapshot(IPTR *flags)
{
    Object  *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    BOOL    snapshot_all = *flags;

    DoMethod(window, MUIM_IconWindow_Snapshot, snapshot_all);
}
///

///wanderer_menufunc_window_view_iconsonly(Object **pstrip)
void wanderer_menufunc_window_view_iconsonly(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_ALL);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if ((item != NULL) && (iconList != NULL))
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

void wanderer_menufunc_window_view_modeicon(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_ICON);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if ((item != NULL) && (iconList != NULL))
    {
        IPTR display_bits = 0, menu_view_state = 0;
        GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);
        
        display_bits &= ~(ICONLIST_DISP_MODEDEFAULT | ICONLIST_DISP_MODELABELRIGHT | ICONLIST_DISP_MODELIST);

        GET(item, MUIA_Menuitem_Checked, &menu_view_state);

        if (menu_view_state == TRUE)
        {
//            if ( != LABELRIGHT)
                display_bits |= ICONLIST_DISP_MODEDEFAULT;
//            else
//                    display_bits |= ICONLIST_DISP_MODELABELRIGHT;
        }

        SET(iconList, MUIA_IconList_DisplayFlags, display_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

void wanderer_menufunc_window_view_modelist(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_DETAIL);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if ((item != NULL) && (iconList != NULL))
    {
        IPTR display_bits = 0, menu_view_state = 0;
        GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);
        
        display_bits &= ~(ICONLIST_DISP_MODEDEFAULT | ICONLIST_DISP_MODELABELRIGHT | ICONLIST_DISP_MODELIST);

        GET(item, MUIA_Menuitem_Checked, &menu_view_state);

        if (menu_view_state == TRUE)
        {
            display_bits |= ICONLIST_DISP_MODELIST;
        }

        SET(iconList, MUIA_IconList_DisplayFlags, display_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

///wanderer_menufunc_window_view_hidden(Object **pstrip)
void wanderer_menufunc_window_view_hidden(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_VIEW_HIDDEN);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if ((item != NULL) && (iconList != NULL))
    {
        IPTR display_bits = 0, menu_view_state = 0;
        GET(iconList, MUIA_IconList_DisplayFlags, &display_bits);

        GET(item, MUIA_Menuitem_Checked, &menu_view_state);

        if (menu_view_state)
        {
            display_bits |= ICONLIST_DISP_SHOWHIDDEN;
        }
        else
        {
            display_bits &= ~ICONLIST_DISP_SHOWHIDDEN;
        }

        SET(iconList, MUIA_IconList_DisplayFlags, display_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}

///wanderer_menufunc_window_sort_enable()
void wanderer_menufunc_window_sort_enable(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_ENABLE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL)
    {
        if (!XGET(item, MUIA_Disabled) && (iconList != NULL))
        {
            IPTR sort_bits = 0;

            GET(iconList, MUIA_IconList_SortFlags, &sort_bits);

            if( XGET(item, MUIA_Menuitem_Checked) )
            {
                sort_bits &= ~(MUIV_IconList_Sort_Orders|MUIV_IconList_Sort_Reverse);
                sort_bits |= MUIV_IconList_Sort_DrawersMixed;
                sort_bits |= MUIV_IconList_Sort_AutoSort;

                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_DATE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits |= MUIV_IconList_Sort_ByDate;
                    }
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_SIZE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits |= MUIV_IconList_Sort_BySize;
                    }
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_TYPE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits |= MUIV_IconList_Sort_ByType;
                    }
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_NAME)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits |= MUIV_IconList_Sort_ByName;
                    }
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_REVERSE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits |= MUIV_IconList_Sort_Reverse;
                    }
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_TOPDRAWERS)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, FALSE);
                    if( XGET(item, MUIA_Menuitem_Checked) )
                    {
                        sort_bits &= ~MUIV_IconList_Sort_DrawersMixed;
                    }
                }
D(bug("[Wanderer] %s: (enable) Setting sort flags %08x\n", __PRETTY_FUNCTION__, sort_bits));
            }
            else
            {
                sort_bits &= ~MUIV_IconList_Sort_AutoSort;

                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_DATE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_SIZE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_TYPE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_NAME)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_REVERSE)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
                if ((item = FindMenuitem(strip, MEN_WINDOW_SORT_TOPDRAWERS)) != NULL)
                {
                    NNSET(item, MUIA_Disabled, TRUE);
                }
D(bug("[Wanderer] %s: (disable) Setting sort flags %08x\n", __PRETTY_FUNCTION__, sort_bits));
            }

            SET(iconList, MUIA_IconList_SortFlags, sort_bits);
            DoMethod(iconList, MUIM_IconList_Sort);
        }
    }
}
///

///wanderer_menufunc_window_sort_name()
void wanderer_menufunc_window_sort_name(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_NAME);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

        sort_bits &= ~MUIV_IconList_Sort_Orders;
        if (checked == TRUE)
        {
            sort_bits |= MUIV_IconList_Sort_ByName;
        }
        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_window_sort_date()
void wanderer_menufunc_window_sort_date(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_DATE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

        sort_bits &= ~MUIV_IconList_Sort_Orders;
        if (checked == TRUE)
        {
            sort_bits |= MUIV_IconList_Sort_ByDate;
        }
        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_window_sort_size()
void wanderer_menufunc_window_sort_size(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_SIZE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

D(bug("[Wanderer]: %s: (start) sort flags %08x\n", __PRETTY_FUNCTION__, sort_bits));
        sort_bits &= ~MUIV_IconList_Sort_Orders;
        if (checked == TRUE)
        {
            sort_bits |= MUIV_IconList_Sort_BySize;
        }
D(bug("[Wanderer]: %s: (end) sort flags %08x\n", __PRETTY_FUNCTION__, sort_bits));
        SET(iconList, MUIA_IconList_SortFlags, sort_bits);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_window_sort_type()
void wanderer_menufunc_window_sort_type(Object **pstrip)
{
    Object *strip = *pstrip;
    Object *item = FindMenuitem(strip, MEN_WINDOW_SORT_TYPE);
    Object *window = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

        sort_bits &= ~MUIV_IconList_Sort_Orders;
        if (checked == TRUE)
        {
            sort_bits |= MUIV_IconList_Sort_ByType;
        }
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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

        if (checked == TRUE)
        {
            sort_bits |= MUIV_IconList_Sort_Reverse;
        }
        else
        {
            sort_bits &= ~MUIV_IconList_Sort_Reverse;
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

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if (item != NULL && iconList != NULL)
    {
        IPTR sort_bits = 0, checked = FALSE;
        GET(iconList, MUIA_IconList_SortFlags, &sort_bits);
        GET(item, MUIA_Menuitem_Checked, &checked);

        if (checked == TRUE)
        {
            sort_bits &= ~MUIV_IconList_Sort_DrawersMixed;
        }
        else
        {
            sort_bits |= MUIV_IconList_Sort_DrawersMixed;
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
    struct IconList_Entry *entry    = (APTR) MUIV_IconList_NextIcon_Start;

    do
    {
        DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR) &entry);

        if ((IPTR)entry != MUIV_IconList_NextIcon_End)
        {
            if (entry->type == ILE_TYPE_APPICON)
                continue; /* TODO: Implement */

            {
                BPTR lock   = Lock(entry->ile_IconEntry->ie_IconNode.ln_Name, ACCESS_READ);
                BPTR parent = ParentDir(lock);
                UnLock(lock);

                D(bug("[Wanderer] %s: selected = '%s'\n", __PRETTY_FUNCTION__,
                        entry->ile_IconEntry->ie_IconNode.ln_Name));

                OpenWorkbenchObject
                (
                    "WANDERER:Tools/WBRename",
                    WBOPENA_ArgLock, (IPTR) parent,
                    WBOPENA_ArgName, (IPTR) FilePart(entry->ile_IconEntry->ie_IconNode.ln_Name),
                    TAG_DONE
                );

                D(bug("[Wanderer] %s: selected = '%s'\n", __PRETTY_FUNCTION__,
                        entry->ile_IconEntry->ie_IconNode.ln_Name));

                UnLock(parent);
            }
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
    struct IconList_Entry *entry    = (IPTR)MUIV_IconList_NextIcon_Start;

    D(bug("[Wanderer] %s: Window @ %p, IconList @ %p\n", __PRETTY_FUNCTION__, window, iconList));
        
    do
    {
        DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

        if ((IPTR)entry != MUIV_IconList_NextIcon_End)
        {
            if (entry->type == ILE_TYPE_APPICON)
                continue; /* TODO: Implement */

            {
                BPTR lock, parent;
                STRPTR name, file;

                file = entry->ile_IconEntry->ie_IconNode.ln_Name;
                D(bug("[Wanderer] %s: selected = '%s'\n", __PRETTY_FUNCTION__, file));

attemptlock:

                D(bug("[Wanderer] %s: Trying with '%s'\n", __PRETTY_FUNCTION__, file));

                if ((lock = Lock(file, ACCESS_READ)) == BNULL)
                {
                    D(bug("[Wanderer] %s: couldnt lock '%s'\n", __PRETTY_FUNCTION__, file));
                    if ((strlen(file) > 5)
                        && (strcmp(file + strlen(file) - 5, ".info") != 0)
                        && (file[strlen(file) -1] != ':'))
                    {
                        D(bug("[Wanderer] %s: not a '.info' file or device - check if there is a '.info'..\n",
                                __PRETTY_FUNCTION__));
                        file = AllocVec(strlen(entry->ile_IconEntry->ie_IconNode.ln_Name) + 6, MEMF_CLEAR);
                        sprintf(file, "%s.info", entry->ile_IconEntry->ie_IconNode.ln_Name);
                        goto attemptlock;
                    }
                }
                else
                {
                    name = FilePart(file);
                    if (name[0])
                    {
                        parent = ParentDir(lock);
                        UnLock(lock);
                    }
                    else
                    {
                        parent = lock;
                    }

                    D(bug("[Wanderer] %s: Calling WBInfo(name = '%s' parent lock = 0x%p)\n",
                            __PRETTY_FUNCTION__, name, lock));
                    WBInfo(parent, name, NULL);

                    UnLock(parent);
                }
                if ((char *)file != entry->ile_IconEntry->ie_IconNode.ln_Name)
                {
                    FreeVec(file);
                }
            }
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
    Object                      *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
    Object                      *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry       *entry    = (IPTR)MUIV_IconList_NextIcon_Start;
    struct IconEntry            *node = NULL;
    BOOL                        snapshot  = *flags;
    struct TagItem              icontags[] = 
    {
        { ICONPUTA_OnlyUpdatePosition, TRUE },
        { TAG_DONE, 0                       }
    };

    D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    do
    {
        DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);
        
        if ((IPTR)entry != MUIV_IconList_NextIcon_End)
        {
            if (entry->type == ILE_TYPE_APPICON)
                continue; /* TODO: Implement */

            /* On 3.1 Workbench it is not possible to snapshot an object that does not have icon file. Wanderer in such
             * case automatically creates and icon because user explicitly request icon snapshot operation
             * (see window snapshoting comment) */

            node = (struct IconEntry *)((IPTR)entry - ((IPTR)&node->ie_IconListEntry - (IPTR)node));
            D(bug("[Wanderer] %s: %s entry = '%s' @ %p, (%p)\n", __PRETTY_FUNCTION__,
                    (snapshot) ? "SNAPSHOT" : "UNSNAPSHOT", entry->ile_IconEntry->ie_IconNode.ln_Name, entry, node));
            if (node->ie_DiskObj)
            {
                if (snapshot)
                {
                    node->ie_DiskObj->do_CurrentX = node->ie_IconX;
                    node->ie_DiskObj->do_CurrentY = node->ie_IconY;
                }
                else
                {
                    node->ie_DiskObj->do_CurrentX = NO_ICON_POSITION;
                    node->ie_DiskObj->do_CurrentY = NO_ICON_POSITION;
                }
                PutIconTagList(entry->ile_IconEntry->ie_IconNode.ln_Name, node->ie_DiskObj, icontags);
                D(bug("[Wanderer] %s: saved ..\n", __PRETTY_FUNCTION__));
            }
            else
            {
                D(bug("[Wanderer] %s: icon has no diskobj!\n", __PRETTY_FUNCTION__));
            }
        }
        else
        {
            break;
        }
    } while (TRUE);
    D(bug("[Wanderer] %s: finished ..\n", __PRETTY_FUNCTION__));
}

#define BDRPLINELEN_MAX 1024

struct DesktopLinkIcon_Entry
{
    struct Node dlie_Node;
};

///wanderer_menufunc_icon_leaveout()
void wanderer_menufunc_icon_leaveout(void)
{
    Object                                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
    Object                                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    Object                                *rootwindow   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_WorkbenchWindow);
    Object                                *rooticonList = (Object *) XGET(rootwindow, MUIA_IconWindow_IconList);
    struct IconList_Entry                 *entry    = (IPTR)MUIV_IconList_NextIcon_Start;
    // struct IconEntry                      *node     = NULL;
    char                                *leavout_dir = NULL;
    struct DesktopLinkIcon_Entry        *bdrpeNode = NULL, *loiEntry = NULL;

    D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    GET(window, MUIA_IconWindow_Location, &leavout_dir);

    if (leavout_dir != NULL)
    {
        D(bug("[Wanderer] %s: dir '%s'\n", __PRETTY_FUNCTION__, leavout_dir));
        
        char *entryVolume = NULL;
        int i;

        for (i = 0; i < strlen(leavout_dir); i++)
        {
            if (leavout_dir[i] == ':')
            {
                entryVolume = AllocVec(i + 2, MEMF_CLEAR);
                CopyMem(leavout_dir, entryVolume, i + 1);
            }
        }
        
        if (entryVolume != NULL)
        {
            D(bug("[Wanderer] %s: Updating .backdrop file for volume '%s'.. \n", __PRETTY_FUNCTION__, entryVolume));
            char * bdrp_file = NULL;

            if ((bdrp_file = AllocVec(strlen(entryVolume) + 9 + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
            {
                struct List bdrp_Entries;
                BPTR         bdrp_lock = BNULL;
                BOOL        bdrp_changed = FALSE;
                char        *linebuf = NULL;

                sprintf(bdrp_file, "%s.backdrop", entryVolume);

                NEWLIST(&bdrp_Entries);
                
                if ((bdrp_lock = Open(bdrp_file, MODE_OLDFILE)))
                {
                    D(bug("[Wanderer] %s:    Parsing backdrop file: '%s'\n", __PRETTY_FUNCTION__, bdrp_file));

                    if ((linebuf = AllocMem(BDRPLINELEN_MAX, MEMF_PUBLIC)) != NULL)
                    {
                        while (FGets(bdrp_lock, linebuf, BDRPLINELEN_MAX))
                        {
                            bdrpeNode = AllocMem(sizeof(struct DesktopLinkIcon_Entry), MEMF_CLEAR);
                            bdrpeNode->dlie_Node.ln_Name = AllocVec(strlen(linebuf) + 1, MEMF_CLEAR);
                            CopyMem(linebuf, bdrpeNode->dlie_Node.ln_Name, strlen(linebuf) - 1);

                            D(bug("[Wanderer] %s:       Existing entry '%s'\n", __PRETTY_FUNCTION__,
                                    bdrpeNode->dlie_Node.ln_Name));

                            AddTail(&bdrp_Entries, &bdrpeNode->dlie_Node);
                        }
                    }
                    Close(bdrp_lock);
                }

                do
                {
                    DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

                    if (((IPTR)entry != MUIV_IconList_NextIcon_End) && ((entry->type == ST_FILE) || (entry->type == ST_USERDIR)))
                    {
                        char *entryDIEString = (entry->ile_IconEntry->ie_IconNode.ln_Name + strlen(entryVolume) - 1);
                        D(bug("[Wanderer] %s:    Leave-Out Entry = '%s' @ %p ('%s')\n", __PRETTY_FUNCTION__,
                                entry->ile_IconEntry->ie_IconNode.ln_Name, entry, entryDIEString));
                        loiEntry = NULL;

                        ForeachNode(&bdrp_Entries, bdrpeNode)
                        {

                            if (strcmp(bdrpeNode->dlie_Node.ln_Name, entryDIEString) == 0)
                            {
                                loiEntry = bdrpeNode;
                                break;
                            }

                        }

                        if (loiEntry == NULL)
                        {
                            bdrpeNode = AllocMem(sizeof(struct DesktopLinkIcon_Entry), MEMF_CLEAR);
                            bdrpeNode->dlie_Node.ln_Name = AllocVec(strlen(entryDIEString) + 2, MEMF_CLEAR);
                            CopyMem(entryDIEString, bdrpeNode->dlie_Node.ln_Name, strlen(entryDIEString));

                            D(bug("[Wanderer] %s:       Created new .backdrop entry for '%s'\n",
                                    __PRETTY_FUNCTION__, bdrpeNode->dlie_Node.ln_Name));

                            AddTail(&bdrp_Entries, &bdrpeNode->dlie_Node);
                            bdrp_changed = TRUE;
                        }
                    }
                    else
                    {
                        break;
                    }
                } while (TRUE);
            
                if (bdrp_changed)
                {
                    D(bug("[Wanderer] %s:    Updating backdrop file for '%s' ..\n", __PRETTY_FUNCTION__, entryVolume));
                    // Write out the new backdrop file :
                    //   this will cause a filesystem notification in the iconwindow_volumeiconlist
                    //   class that will cause the changes to be noticed if the underlying
                    //   filesystem supports it....

                    if ((bdrp_lock = Open(bdrp_file, MODE_NEWFILE)))
                    {
                        ForeachNode(&bdrp_Entries, bdrpeNode)
                        {
                            D(bug("[Wanderer] %s:       Writing entry '%s'\n", __PRETTY_FUNCTION__,
                                    bdrpeNode->dlie_Node.ln_Name));
                            bdrpeNode->dlie_Node.ln_Name[strlen(bdrpeNode->dlie_Node.ln_Name)] = '\n';
                            FPuts(bdrp_lock, bdrpeNode->dlie_Node.ln_Name);
                        }
                        Close(bdrp_lock);
                    }
                    struct List                *iconList = NULL;
                    struct IconEntry        *entry = NULL;

                    GET(rooticonList, MUIA_Family_List, &iconList);
                    ForeachNode(iconList, entry)
                    {
                        if ((entry->ie_IconListEntry.type == ST_ROOT)
                            && (strncmp(entry->ie_IconListEntry.label, entryVolume, strlen(entry->ie_IconListEntry.label)) == 0))
                        {
                            struct VolumeIcon_Private   *volPrivate = entry->ie_IconListEntry.udata;;
                            if (volPrivate && (volPrivate->vip_FSNotifyRequest.nr_Name == NULL))
                            {
                                //Volumes filesystem couldnt handle fs notifications so we will have to force the update ..
                                D(bug("[Wanderer] %s:    Forcing desktop redraw for volume '%s'\n",
                                        __PRETTY_FUNCTION__, entryVolume));
                                DoMethod(rooticonList, MUIM_IconList_Update);
                                DoMethod(rooticonList, MUIM_IconList_Sort);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    D(bug("[Wanderer] %s: finished ..\n", __PRETTY_FUNCTION__));
}

struct PutAwayIcon_Volume
{
    struct Node paiv_Node;
    struct List paiv_Entries;
};

///wanderer_menufunc_icon_putaway()
void wanderer_menufunc_icon_putaway(void)
{
    Object                        *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);   
    Object                        *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    Object                        *rootwindow   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_WorkbenchWindow);
    Object                        *rooticonList = (Object *) XGET(rootwindow, MUIA_IconWindow_IconList);
    struct IconList_Entry         *entry    = (IPTR)MUIV_IconList_NextIcon_Start;
    struct IconEntry              *node = NULL;
    struct PutAwayIcon_Volume        *paivNode = NULL, *paiVolume = NULL;
    struct DesktopLinkIcon_Entry        *bdrpeNode = NULL, *paieNode = NULL, *paiEntry = NULL;
    struct List                    putawayiconlists;

    D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    NEWLIST(&putawayiconlists);

    do
    {
        DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&entry);

        if (((IPTR)entry != MUIV_IconList_NextIcon_End) && ((entry->type == ST_LINKFILE) || (entry->type == ST_LINKDIR)))
        {
            node = (struct IconEntry *)((IPTR)entry - ((IPTR)&node->ie_IconListEntry - (IPTR)node));
            D(bug("[Wanderer] %s: entry = '%s' @ %p, (%p)\n", __PRETTY_FUNCTION__,
                    entry->ile_IconEntry->ie_IconNode.ln_Name, entry, node));

            char *entryVolume = NULL;
            int i;

            for (i = 0; i < strlen(entry->ile_IconEntry->ie_IconNode.ln_Name); i++)
            {
                if (entry->ile_IconEntry->ie_IconNode.ln_Name[i] == ':')
                {
                    entryVolume = AllocVec(i + 2, MEMF_CLEAR);
                    CopyMem(entry->ile_IconEntry->ie_IconNode.ln_Name, entryVolume, i + 1);
                }
            }
            
            if (entryVolume == NULL)
            {
                D(bug("[Wanderer] %s: couldnt figure out the volume name.. ?????\n", __PRETTY_FUNCTION__));
                continue;
            }
            
            ForeachNode(&putawayiconlists, paivNode)
            {
                if (strcmp(paivNode->paiv_Node.ln_Name, entryVolume) == 0)
                {
                    paiVolume = paivNode;
                }
            }
            
            if (paiVolume == NULL)
            {
                paiVolume = AllocMem(sizeof(struct PutAwayIcon_Volume), MEMF_CLEAR);
                paiVolume->paiv_Node.ln_Name = entryVolume;
                NEWLIST(&paiVolume->paiv_Entries);
                AddTail(&putawayiconlists, &paiVolume->paiv_Node);
            }
            else
            {
                FreeVec(entryVolume);
            }
            
            paiEntry = AllocMem(sizeof(struct DesktopLinkIcon_Entry), MEMF_CLEAR);
            paiEntry->dlie_Node.ln_Name = entry->ile_IconEntry->ie_IconNode.ln_Name + strlen(paiVolume->paiv_Node.ln_Name) - 1;

            AddTail(&paiVolume->paiv_Entries, &paiEntry->dlie_Node);
        }
        else if ((IPTR)entry == MUIV_IconList_NextIcon_End)
        {
            break;
        }
    } while (TRUE);

    ForeachNode(&putawayiconlists, paivNode)
    {
        // Open the Backdrop file and read in the existing contents ...
        char *bdrp_file = NULL;

        D(bug("[Wanderer] %s: Processing entries in .backdrop file for '%s' ..\n", __PRETTY_FUNCTION__, paivNode->paiv_Node.ln_Name));
        if ((bdrp_file = AllocVec(strlen(paivNode->paiv_Node.ln_Name) + 9 + 1, MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
        {
            struct List bdrp_Entries;
            BPTR         bdrp_lock = BNULL;
            BOOL        bdrp_changed = FALSE;
            char        *linebuf = NULL;

            sprintf(bdrp_file, "%s.backdrop", paivNode->paiv_Node.ln_Name);

            NEWLIST(&bdrp_Entries);
            
            if ((bdrp_lock = Open(bdrp_file, MODE_OLDFILE)))
            {
                D(bug("[Wanderer] %s:    Parsing backdrop file: '%s'\n", __PRETTY_FUNCTION__, bdrp_file));

                if ((linebuf = AllocMem(BDRPLINELEN_MAX, MEMF_PUBLIC)) != NULL)
                {
                    while (FGets(bdrp_lock, linebuf, BDRPLINELEN_MAX))
                    {
                        bdrpeNode = AllocMem(sizeof(struct DesktopLinkIcon_Entry), MEMF_CLEAR);
                        bdrpeNode->dlie_Node.ln_Name = AllocVec(strlen(linebuf) + 1, MEMF_CLEAR);
                        CopyMem(linebuf, bdrpeNode->dlie_Node.ln_Name, strlen(linebuf) - 1);

                        D(bug("[Wanderer] %s:       Existing entry '%s'\n", __PRETTY_FUNCTION__,
                                bdrpeNode->dlie_Node.ln_Name));

                        AddTail(&bdrp_Entries, &bdrpeNode->dlie_Node);
                    }
                }
                Close(bdrp_lock);
            }
            
            ForeachNode(&paivNode->paiv_Entries, paieNode)
            {
                paiEntry = NULL;
                bdrpeNode = NULL;
                D(bug("[Wanderer] %s:    Checking for '%s' ..\n", __PRETTY_FUNCTION__, paieNode->dlie_Node.ln_Name));

                ForeachNode(&bdrp_Entries, bdrpeNode)
                {
                    
                    if (strcmp(paieNode->dlie_Node.ln_Name, bdrpeNode->dlie_Node.ln_Name) == 0)
                    {
                        paiEntry = bdrpeNode;
                        break;
                    }
                        
                }
                //Find and remove the entry ...
                if (paiEntry != NULL)
                {
                    D(bug("[Wanderer] %s:       Removing entry '%s'\n", __PRETTY_FUNCTION__, paiEntry->dlie_Node.ln_Name));
                    Remove(&paiEntry->dlie_Node);
                    bdrp_changed = TRUE;
                }
            }
            if (bdrp_changed)
            {
                D(bug("[Wanderer] %s:    Updating backdrop file for '%s' ..\n", __PRETTY_FUNCTION__,
                        paivNode->paiv_Node.ln_Name));
                // Write out the new backdrop file :
                // this will cause a filesystem notification in the iconwindow_volumeiconlist
                // class that will cause the changes to be noticed ....
                if ((bdrp_lock = Open(bdrp_file, MODE_NEWFILE)))
                {
                    ForeachNode(&bdrp_Entries, bdrpeNode)
                    {
                        D(bug("[Wanderer] %s:       Writing entry '%s'\n", __PRETTY_FUNCTION__,
                                bdrpeNode->dlie_Node.ln_Name));
                        bdrpeNode->dlie_Node.ln_Name[strlen(bdrpeNode->dlie_Node.ln_Name)] = '\n';
                        FPuts(bdrp_lock, bdrpeNode->dlie_Node.ln_Name);
                    }
                    Close(bdrp_lock);
                }
                struct List                *iconList = NULL;
                struct IconEntry        *entry = NULL;

                GET(rooticonList, MUIA_Family_List, &iconList);
                ForeachNode(iconList, entry)
                {
                    if ((entry->ie_IconListEntry.type == ST_ROOT)
                        && (strncmp(entry->ie_IconListEntry.label, paivNode->paiv_Node.ln_Name, strlen(entry->ie_IconListEntry.label)) == 0))
                    {
                        struct VolumeIcon_Private   *volPrivate = entry->ie_IconListEntry.udata;;
                        if (volPrivate && (volPrivate->vip_FSNotifyRequest.nr_Name == NULL))
                        {
                            //Volumes filesystem couldnt handle fs notifications so we will have to force the update ..
                            D(bug("[Wanderer] %s:    Forcing desktop redraw for volume '%s'\n", __PRETTY_FUNCTION__,
                                    paivNode->paiv_Node.ln_Name));
                            DoMethod(rooticonList, MUIM_IconList_Update);
                            DoMethod(rooticonList, MUIM_IconList_Sort);
                            break;
                        }
                    }
                }
            }
        }
    }
    D(bug("[Wanderer] %s: Finished\n", __PRETTY_FUNCTION__));
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
        MUIA_Application_Title,         (IPTR)wand_copyprocnamestr,
        MUIA_Application_Base,          (IPTR)"WANDERER_COPY",
        MUIA_Application_Copyright,     (IPTR) wand_copyrightstr,
        MUIA_Application_Author,        (IPTR) wand_authorstr,
        MUIA_Application_SingleTask,    (IPTR)FALSE,
        MUIA_Application_Window,        (IPTR)(d->win = MUI_NewObject(MUIC_Window,
            MUIA_Window_Title,          (IPTR)_(MSG_WANDERER_FILEACCESS),
            MUIA_Window_ID,             (IPTR)MAKE_ID('W','F','C','R'),
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
            WindowContents,             (group = MUI_NewObject(MUIC_Group,
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
                        MUIA_Gauge_InfoText, _(MSG_WANDERER_FILEACCESS_PROCESSING),
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

                Child, (IPTR)( d->stopObject = SimpleButton( _(MSG_WANDERER_FILEACCESS_STOP) ) ),
            TAG_DONE)),
        TAG_DONE)),
    TAG_DONE);

    if (d->copyApp) 
    {
        if ((flags & (ACTION_COPY|ACTION_DELETE)) == (ACTION_COPY|ACTION_DELETE)) 
        {
            SET(fromObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_MOVEFROM) );
            SET(toObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_MOVETO) );
            SET(fileTextObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_FILE) );
            SET(fileLengthObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_TRAFFIC) );
        } 
        else if ((flags & ACTION_COPY) == ACTION_COPY) 
        {
            SET(fromObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_COPYFROM) );
            SET(toObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_COPYTO) );
            SET(fileTextObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_FILE) );
            SET(fileLengthObject, MUIA_Text_Contents, (IPTR) _(MSG_WANDERER_FILEACCESS_TRAFFIC) );

        } 
        else if ((flags & ACTION_DELETE) == ACTION_DELETE) 
        {
            SET(fromObject, MUIA_Text_Contents, _(MSG_WANDERER_FILEACCESS_DELETEFROM) );
            DoMethod(group, MUIM_Group_InitChange);
            DoMethod(group, OM_REMMEMBER, toObject);
            DoMethod(group, OM_REMMEMBER, fileLengthObject);
            DoMethod(group, OM_REMMEMBER, d->performanceObject);
            DoMethod(group, OM_REMMEMBER, d->destObject);
            DoMethod(group, OM_REMMEMBER, gaugeGroup);
            DoMethod(group, MUIM_Group_ExitChange);
            SET(fileTextObject, MUIA_Text_Contents, _(MSG_WANDERER_FILEACCESS_FILETODELETE) );
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
    struct IconList_Entry *entry    = ( void*) MUIV_IconList_NextIcon_Start;
    struct MUIDisplayObjects dobjects;
    struct Hook displayCopyHook;
    struct Hook displayAskHook;
    struct OpModes opModes;
    ULONG updatedIcons;

    DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR) &entry);
    displayCopyHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_DisplayCopyFunc;
    displayAskHook.h_Entry = (HOOKFUNC) Wanderer__HookFunc_AskModeFunc;
    opModes.askhook = &displayAskHook;
    opModes.deletemode = OPMODE_ASK;
    opModes.protectmode = OPMODE_ASK;
    opModes.overwritemode = OPMODE_ASK;

    updatedIcons = 0;

    /* Process all selected entries */
    if (CreateCopyDisplay(ACTION_DELETE, &dobjects))
    {
        do
        {   
            if ((IPTR)entry != MUIV_IconList_NextIcon_End)
            {
                if (entry->type != ILE_TYPE_APPICON) /* TODO: Implement */
                {
                    /* copy via filesystems.c */
                    D(bug("[Wanderer] Delete \"%s\"\n", entry->ile_IconEntry->ie_IconNode.ln_Name);)
                    CopyContent( NULL, entry->ile_IconEntry->ie_IconNode.ln_Name, NULL, TRUE,
                            ACTION_DELETE, &displayCopyHook, &opModes, (APTR) &dobjects);
                    updatedIcons++;
                }
            }
            DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR) &entry);
        } 
        while ((IPTR)entry != MUIV_IconList_NextIcon_End );
        DisposeCopyDisplay(&dobjects);
    }
    // Only update list if anything happened to the icons!
    if ( updatedIcons > 0 )
    {
        DoMethod(window, MUIM_IconWindow_UnselectAll);
        DoMethod(iconList, MUIM_IconList_Update);
        DoMethod(iconList, MUIM_IconList_Sort);
    }
}
///

///wanderer_menufunc_icon_format()
void wanderer_menufunc_icon_format(void)
{
    Object                *window   = (Object *) XGET(_WandererIntern_AppObj, MUIA_Wanderer_ActiveWindow);
    Object                *iconList = (Object *) XGET(window, MUIA_IconWindow_IconList);
    struct IconList_Entry *entry    = ( void*) MUIV_IconList_NextIcon_Start;

    DoMethod(iconList, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR) &entry);

    /* Process only first selected entry */
    if ((IPTR)entry != MUIV_IconList_NextIcon_End)
    {  
        if (entry->type != ILE_TYPE_APPICON) /* TODO: Implement */
        {
            BPTR lock   = Lock(entry->ile_IconEntry->ie_IconNode.ln_Name, ACCESS_READ);
            D(bug("[Wanderer]: %s('%s')\n", __PRETTY_FUNCTION__, entry->ile_IconEntry->ie_IconNode.ln_Name));
            /* Usually we pass object name and parent lock. Here we do the same thing.
               Just object name is empty string and its parent is device's root. */
            OpenWorkbenchObject
              (
                "SYS:System/Format",
                WBOPENA_ArgLock, (IPTR) lock,
                WBOPENA_ArgName, lock ? (IPTR)"" : (IPTR)entry->ile_IconEntry->ie_IconNode.ln_Name,
                TAG_DONE
              );
            if (lock)
                UnLock(lock);
        }
    }
}
///

///wanderer_menufunc_wanderer_AROS_guisettings()
void wanderer_menufunc_wanderer_AROS_guisettings(void)
{
    //DoMethod(_WandererIntern_AppObj, MUIM_Application_OpenConfigWindow);
    OpenWorkbenchObject("SYS:Prefs/Zune",
                WBOPENA_ArgName, (IPTR) "WANDERER",
                TAG_DONE);
}
///

///wanderer_menufunc_wanderer_AROS_about()
void wanderer_menufunc_wanderer_AROS_about(void)
{
    OpenWorkbenchObject("SYS:System/About",
                TAG_DONE);
}
///

///wanderer_menufunc_wanderer_about()
void wanderer_menufunc_wanderer_about(Object **pwand)
{
    Object *self = *pwand;
    Class *CLASS = _WandererIntern_CLASS;
    SETUP_WANDERER_INST_DATA;

    /* Display Information about this version of wanderer */
    if (data->wd_AboutWindow == NULL)
    {
        data->wd_AboutWindow = WindowObject,
            MUIA_Window_Title,          (IPTR)"About Wanderer...",
            MUIA_Window_ID,             (IPTR)MAKE_ID('W','A','B','T'),
            WindowContents, (IPTR)VGroup,
                Child, (IPTR)HGroup,
                        Child, (IPTR) IconImageObject,
                            MUIA_InputMode, MUIV_InputMode_Toggle,
                            MUIA_IconImage_File, (IPTR)"PROGDIR:Wanderer",
                        End,
                    Child, (IPTR)VGroup,
                        Child, (IPTR)TextObject,
                            MUIA_Text_PreParse, (IPTR)"\33c\33b", 
                            MUIA_Text_Contents, (IPTR)wand_titlestr,
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_PreParse, (IPTR)"\33c", 
                            MUIA_Text_Contents, (IPTR)wand_copyrightstr,
                        End,
                    End,
                End,

                Child, (IPTR)VGroup,
                    MUIA_Frame, MUIV_Frame_Group,
                    Child, (IPTR)TextObject,
                        MUIA_Text_PreParse, (IPTR)"\33b", 
                        MUIA_Text_Contents, (IPTR)"Internal Classes:",
                    End,
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)ColGroup(2),
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconWindow",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconWindowVolumeList",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconWindowDrawerList",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                    End,
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)TextObject,
                        MUIA_Text_PreParse, (IPTR)"\33b", 
                        MUIA_Text_Contents, (IPTR)"External Classes:",
                    End,
                    Child, (IPTR)HVSpace,
                    Child, (IPTR)ColGroup(2),
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconList.mui",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconListView.mui",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconVolumeList.mui",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"IconDrawerList.mui",
                        End,
                        Child, (IPTR)TextObject,
                            MUIA_Text_Contents, (IPTR)"",
                        End,
                    End,
                    Child, (IPTR)HVSpace,
                End,
            End,
        End;

#ifdef __AROS__
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) data->wd_AboutWindow);
#else
        DoMethod(self, OM_ADDMEMBER, (IPTR) data->wd_AboutWindow);
#endif
        DoMethod
        (
            data->wd_AboutWindow, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, 
            (IPTR)data->wd_AboutWindow, 3, MUIM_Set, MUIA_Window_Open, FALSE
        );
    }

    if (data->wd_AboutWindow)
    {
        IPTR isOpen = (IPTR)FALSE;
        GET(data->wd_AboutWindow, MUIA_Window_Open, &isOpen);
        if (isOpen)
        {
            isOpen = FALSE;
        }
        else
        {
            isOpen = TRUE;
        }
        SET(data->wd_AboutWindow, MUIA_Window_Open, isOpen);
    }
}
///

///wanderer_menufunc_wanderer_quit()
void wanderer_menufunc_wanderer_quit(void)
{
    if (OpenWorkbenchObject("WANDERER:Tools/Quit", TAG_DONE))
    {
        ;
    }
    else
    {
        if (MUI_RequestA(_WandererIntern_AppObj, NULL, 0, wand_namestr, _(MSG_YESNO), _(MSG_REALLYQUIT), NULL))
        DoMethod(_WandererIntern_AppObj, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
    
    }
}
///

///wanderer_menufunc_wanderer_shutdown()
void wanderer_menufunc_wanderer_shutdown(void)
{
    LONG action;

#if defined(__AROS_ARCH_pc__) && defined(__i386__)
    action = MUI_RequestA(_WandererIntern_AppObj, NULL, 0,
        _(MSG_SHUTDOWN_TITLE), strchr(_(MSG_SHUTDOWN_BUTTONS), '|') + 1,
        _(MSG_SHUTDOWN_BODY), NULL);
    if (action != 0)
        action++;
#else
    action = MUI_RequestA(_WandererIntern_AppObj, NULL, 0, _(MSG_SHUTDOWN_TITLE), _(MSG_SHUTDOWN_BUTTONS), _(MSG_SHUTDOWN_BODY), NULL);
#endif
    switch (action) {
    case 0:
        return;
    case 1:
        ShutdownA(SD_ACTION_POWEROFF);
        break;
    case 2:
        ShutdownA(SD_ACTION_COLDREBOOT);
        break;
    case 3:
        ColdReboot();
    }
    MUI_RequestA(_WandererIntern_AppObj, NULL, 0, _(MSG_SHUTDOWN_TITLE), _(MSG_OK), _(MSG_ACTION_NOT_SUPPORTED), NULL);
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
VOID DoMenuNotify(Object* strip, int id, IPTR trigattrib, void *function, void *arg)
{
    Object *entry;
    entry = FindMenuitem(strip,id);
    if (entry)
    {
        DoMethod
          (
            entry, MUIM_Notify, trigattrib, MUIV_EveryTime, 
            (IPTR) entry, 4, MUIM_CallHook, (IPTR) &_WandererIntern_hook_standard,
            (IPTR) function, (IPTR) arg
          );
    }
}
///

///SetMenuDefaultNotifies()
VOID SetMenuDefaultNotifies(Object *wanderer, Object *strip, STRPTR path)
{
    Object *item;

    if (!strip) return;

    DoMenuNotify(strip, MEN_WANDERER_EXECUTE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_execute, path);
    DoMenuNotify(strip, MEN_WANDERER_SHELL, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_shell, path);
    DoMenuNotify(strip, MEN_WANDERER_AROS_GUISETTINGS, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_AROS_guisettings, NULL);
    DoMenuNotify(strip, MEN_WANDERER_AROS_ABOUT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_AROS_about, NULL);
    DoMenuNotify(strip, MEN_WANDERER_ABOUT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_about, wanderer);
    DoMenuNotify(strip, MEN_WANDERER_QUIT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_quit, NULL);
    DoMenuNotify(strip, MEN_WANDERER_SHUTDOWN, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_wanderer_shutdown, NULL);

    DoMenuNotify(strip, MEN_WINDOW_NEW_DRAWER, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_newdrawer, path);
    DoMenuNotify(strip, MEN_WINDOW_OPEN_PARENT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_openparent, path);
    DoMenuNotify(strip, MEN_WINDOW_CLOSE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_close, NULL);
    DoMenuNotify(strip, MEN_WINDOW_UPDATE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_update, NULL);
    DoMenuNotify(strip, MEN_WINDOW_CLEAR, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_clear, NULL);

    DoMenuNotify(strip, MEN_WINDOW_SNAP_WIN, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_snapshot, FALSE);
    DoMenuNotify(strip, MEN_WINDOW_SNAP_ALL, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_snapshot, (APTR)TRUE);

    DoMenuNotify(strip, MEN_WINDOW_SELECT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_select, NULL);
                                
    DoMenuNotify(strip, MEN_WINDOW_VIEW_ICON, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_view_modeicon, strip);
    DoMenuNotify(strip, MEN_WINDOW_VIEW_DETAIL, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_view_modelist, strip);
    DoMenuNotify(strip, MEN_WINDOW_VIEW_ALL, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_view_iconsonly, strip);
//    DoMenuNotify(strip, MEN_WINDOW_VIEW_HIDDEN, MUIA_Menuitem_Trigger,
//                                wanderer_menufunc_window_view_hidden, strip);

    
    DoMenuNotify(strip, MEN_WINDOW_SORT_NOW, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_cleanup, NULL); 
    DoMenuNotify(strip, MEN_WINDOW_SORT_ENABLE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_enable, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_NAME, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_name, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TYPE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_type, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_DATE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_date, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_SIZE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_size, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_REVERSE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_reverse, strip);
    DoMenuNotify(strip, MEN_WINDOW_SORT_TOPDRAWERS, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_window_sort_topdrawers, strip);

    DoMenuNotify(strip, MEN_ICON_OPEN, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_open, NULL);
    DoMenuNotify(strip, MEN_ICON_RENAME, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_rename, NULL);
    DoMenuNotify(strip, MEN_ICON_INFORMATION, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_information, NULL);
    DoMenuNotify(strip, MEN_ICON_SNAPSHOT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_snapshot, (APTR)TRUE);
    DoMenuNotify(strip, MEN_ICON_UNSNAPSHOT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_snapshot, FALSE);
    DoMenuNotify(strip, MEN_ICON_LEAVEOUT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_leaveout, NULL);
    DoMenuNotify(strip, MEN_ICON_PUTAWAY, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_putaway, NULL);
    DoMenuNotify(strip, MEN_ICON_DELETE, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_delete, NULL);
    DoMenuNotify(strip, MEN_ICON_FORMAT, MUIA_Menuitem_Trigger,
                                wanderer_menufunc_icon_format, NULL);

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

#define MENF_ICON_OPEN          (1 << 0)
#define MENF_ICON_RENAME        (1 << 1)
#define MENF_ICON_INFORMATION   (1 << 2)
#define MENF_ICON_SNAPSHOT      (1 << 3)
#define MENF_ICON_UNSNAPSHOT    (1 << 4)
#define MENF_ICON_LEAVEOUT      (1 << 5)
#define MENF_ICON_PUTAWAY       (1 << 6)
#define MENF_ICON_DELETE        (1 << 7)
#define MENF_ICON_FORMAT        (1 << 8)
#define MENF_ICON_EMPTYTRASH    (1 << 9)

///Wanderer__Func_UpdateMenuStates()
VOID Wanderer__Func_UpdateMenuStates(Object *WindowObj, Object *IconlistObj)
{
    IPTR   isRoot = 0, current_DispFlags = 0, current_SortFlags = 0;
    Object *current_Menustrip = NULL, *current_MenuItem = NULL;
    struct IconList_Entry *icon_entry    = (IPTR)MUIV_IconList_NextIcon_Start;
    ULONG iconmenustate = 0xFFFFFFFF; /* All enabled */
    LONG selectedcount = 0;

    if (IconlistObj == NULL)
        return;

    D(bug("[Wanderer]: %s(IconList @ %p)\n", __PRETTY_FUNCTION__, IconlistObj));

    GET(IconlistObj, MUIA_IconList_SortFlags, &current_SortFlags);
    GET(IconlistObj, MUIA_IconList_DisplayFlags, &current_DispFlags);
    GET(WindowObj, MUIA_Window_Menustrip, &current_Menustrip);
    GET(WindowObj, MUIA_IconWindow_IsRoot, &isRoot);

    D(bug("[Wanderer] %s: Menu @ %p, Display Flags : %x, Sort Flags : %x\n", __PRETTY_FUNCTION__, current_Menustrip, current_DispFlags, current_SortFlags));

    do
    {
        DoMethod(IconlistObj, MUIM_IconList_NextIcon, MUIV_IconList_NextIcon_Selected, (IPTR)&icon_entry);

        if ((IPTR)icon_entry != MUIV_IconList_NextIcon_End)
        {
            if (isRoot && (icon_entry->type == ST_ROOT))
            {
                /* Disks can't be: */
                iconmenustate &= ~MENF_ICON_LEAVEOUT;
                iconmenustate &= ~MENF_ICON_PUTAWAY;
                iconmenustate &= ~MENF_ICON_DELETE;
                iconmenustate &= ~MENF_ICON_EMPTYTRASH;
            }
            if (isRoot && ((icon_entry->type == ST_LINKDIR) || (icon_entry->type == ST_LINKFILE)))
            {
                /* Leave outed icons can't be: */
                iconmenustate &= ~MENF_ICON_LEAVEOUT;
                iconmenustate &= ~MENF_ICON_DELETE;
                iconmenustate &= ~MENF_ICON_FORMAT;
                iconmenustate &= ~MENF_ICON_EMPTYTRASH;
            }
            if (!(isRoot) && ((icon_entry->type == ST_USERDIR) || (icon_entry->type == ST_FILE)))
            {
                /* Normal files/drawers can't be: */
                iconmenustate &= ~MENF_ICON_PUTAWAY;
                iconmenustate &= ~MENF_ICON_FORMAT;
                iconmenustate &= ~MENF_ICON_EMPTYTRASH;
            }
            if (isRoot && (icon_entry->type == ILE_TYPE_APPICON))
            {
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsOpen))
                    iconmenustate &= ~MENF_ICON_OPEN;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsRename))
                    iconmenustate &= ~MENF_ICON_RENAME;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsInformation))
                    iconmenustate &= ~MENF_ICON_INFORMATION;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsSnapshot))
                    iconmenustate &= ~MENF_ICON_SNAPSHOT;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsUnSnapshot))
                    iconmenustate &= ~MENF_ICON_UNSNAPSHOT;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsLeaveOut))
                    iconmenustate &= ~MENF_ICON_LEAVEOUT;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsPutAway))
                    iconmenustate &= ~MENF_ICON_PUTAWAY;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsDelete))
                    iconmenustate &= ~MENF_ICON_DELETE;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsFormatDisk))
                    iconmenustate &= ~MENF_ICON_FORMAT;
                if (!AppIcon_Supports((struct AppIcon *)icon_entry->ile_IconEntry->ie_AppIcon, WBAPPICONA_SupportsEmptyTrash))
                    iconmenustate &= ~MENF_ICON_EMPTYTRASH;
            }
            selectedcount++;
        }
        else
        {
            break;
        }
    } while (TRUE);

    if (selectedcount == 0) iconmenustate = 0x00000000;

    if (current_Menustrip != NULL)
    {
        /* Icon menu */
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_OPEN)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_OPEN));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_RENAME)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_RENAME));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_INFORMATION)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_INFORMATION));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_SNAPSHOT)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_SNAPSHOT));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_UNSNAPSHOT)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_UNSNAPSHOT));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_DELETE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_DELETE));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_FORMAT)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_FORMAT));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_LEAVEOUT)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_LEAVEOUT));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_PUTAWAY)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_PUTAWAY));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_ICON_EMPTYTRASH)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, !!(iconmenustate & MENF_ICON_EMPTYTRASH));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_CLEAR)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Enabled, TRUE);
        }

        /* Window menu */
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_VIEW_ALL)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, !(current_DispFlags & ICONLIST_DISP_SHOWINFO) ? TRUE : FALSE);
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_VIEW_ICON)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, (current_DispFlags & ICONLIST_DISP_MODEDEFAULT) ? TRUE : FALSE);
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_VIEW_DETAIL)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, (current_DispFlags & ICONLIST_DISP_MODELIST) ? TRUE : FALSE);
        }
//        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_VIEW_HIDDEN)) != NULL)
//        {
//            NNSET(current_MenuItem, MUIA_Menuitem_Checked, (current_DispFlags & ICONLIST_DISP_SHOWHIDDEN) ? TRUE : FALSE);
//        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_NAME)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_ByName) ? TRUE : FALSE));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_DATE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_ByDate) ? TRUE : FALSE));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_SIZE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_BySize) ? TRUE : FALSE));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_TYPE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_ByType) ? TRUE : FALSE));
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_REVERSE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_Reverse) == MUIV_IconList_Sort_Reverse) ? TRUE : FALSE);
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_TOPDRAWERS)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_DrawersMixed) == MUIV_IconList_Sort_DrawersMixed) ? FALSE : TRUE);
        }
        if ((current_MenuItem = FindMenuitem(current_Menustrip, MEN_WINDOW_SORT_ENABLE)) != NULL)
        {
            NNSET(current_MenuItem, MUIA_Menuitem_Checked, ((current_SortFlags & MUIV_IconList_Sort_AutoSort) == 0) ? FALSE : TRUE);
        }
    }
}

///Wanderer__HookFunc_UpdateMenuStatesFunc()
#ifdef __AROS__
AROS_UFH3
(
    ULONG, Wanderer__HookFunc_UpdateMenuStatesFunc,
    AROS_UFHA(struct Hook *,    hook,   A0),
    AROS_UFHA(APTR *,           obj,    A2),
    AROS_UFHA(APTR,             param,  A1)
)
{
#else
HOOKPROTO(Wanderer__HookFunc_UpdateMenuStatesFunc, ULONG, struct dCopyStruct *obj, APTR param)
{
#endif
    AROS_USERFUNC_INIT

    __unused Object        *self = ( Object *)obj;
    Object        *window = *( Object **)param;
    Object        *iconlist = NULL;

D(bug("[Wanderer]: %s(self @ %p, window @ %p)\n", __PRETTY_FUNCTION__, self, window));

    GET(window, MUIA_IconWindow_IconList, &iconlist);

D(bug("[Wanderer] %s: iconlist @ %p\n", __PRETTY_FUNCTION__, iconlist));

    Wanderer__Func_UpdateMenuStates(window, iconlist);

D(bug("[Wanderer] %s: Update Complete.\n", __PRETTY_FUNCTION__));

    return 0;

    AROS_USERFUNC_EXIT
}

/*** Methods ****************************************************************/
///OM_NEW()
Object *Wanderer__OM_NEW(Class *CLASS, Object *self, struct opSet *message)
{
D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title,       (IPTR) wand_namestr,
        MUIA_Application_Base,        (IPTR) "WANDERER",
        MUIA_Application_Version,     (IPTR) VERSION,
        MUIA_Application_Description, (IPTR) _(MSG_DESCRIPTION),
        MUIA_Application_SingleTask,         TRUE,

        MUIA_Application_Version, (IPTR) wand_versionstr,
        MUIA_Application_Copyright, (IPTR) wand_copyrightstr,
        MUIA_Application_Author, (IPTR) wand_authorstr,

        TAG_MORE, (IPTR) message->ops_AttrList
    );

    if (self != NULL)
    {
        SETUP_WANDERER_INST_DATA;

    //    ULONG updatedIcons;
D(bug("[Wanderer] %s: Wanderer Obj @ %p, Instance data @ %p\n", __PRETTY_FUNCTION__, self, data));

        _WandererIntern_CLASS = CLASS;

        NewList(&_WandererIntern_FSHandlerList);

D(bug("[Wanderer] %s: FSHandlerList @ %p\n", __PRETTY_FUNCTION__, &_WandererIntern_FSHandlerList));

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
        data->wd_TimerIHN.ihn_Millis = 1000;
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

#ifdef __AROS__
        data->wd_Prefs = (Object *)WandererPrefsObject,
                                        MUIA_Wanderer_FileSysNotifyPort, (IPTR)data->wd_NotifyPort,
                                    End; // FIXME: error handling
#else
        data->wd_Prefs = NewObject(WandererPrefs_CLASS->mcc_Class, NULL, TAG_DONE); // FIXME: error handling
#endif

        if (data->wd_Prefs)
        {
            D(bug("[Wanderer] %s: Prefs-Screentitle = '%s'\n", __PRETTY_FUNCTION__, XGET(data->wd_Prefs, MUIA_IconWindowExt_ScreenTitle_String)));
        }
    }

    D(bug("[Wanderer] %s: WandererObj @ %p\n", __PRETTY_FUNCTION__, self));
    return self;
}
///

///OM_DISPOSE()
IPTR Wanderer__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    SETUP_WANDERER_INST_DATA;

    if (data->wd_CommandPort)
    {
        /* InputHandler's have only been added if the creation
           of the msg port was successful */
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_TimerIHN);
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_CommandIHN);
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->wd_NotifyIHN);

        UnregisterWorkbench(data->wd_CommandPort);

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
D(bug("[Wanderer] %s: MUIA_Wanderer_Screen = %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
D(bug("[Wanderer] %s: setting MUIA_Wanderer_Screen isnt yet handled!\n", __PRETTY_FUNCTION__));
            break;

        case MUIA_Wanderer_ActiveWindow:
            data->wd_ActiveWindow = (Object *) tag->ti_Data;
D(bug("[Wanderer] %s: MUIA_Wanderer_ActiveWindow = %p\n", __PRETTY_FUNCTION__, tag->ti_Data));
            if (!(XGET(data->wd_ActiveWindow, MUIA_Window_Activate)))
            {
                NNSET(data->wd_ActiveWindow, MUIA_Window_Activate, TRUE);
            }
            Object *activatewin_Iconlist = NULL;

            GET(data->wd_ActiveWindow, MUIA_IconWindow_IconList, &activatewin_Iconlist);
            Wanderer__Func_UpdateMenuStates(data->wd_ActiveWindow, activatewin_Iconlist);
            break;

        case MUIA_Application_Iconified:
            /* Wanderer does not allow iconifying of itself */
            tag->ti_Tag = TAG_IGNORE;
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

        case MUIA_Wanderer_FileSysNotifyList:
            *store = (IPTR)&_WandererIntern_FSHandlerList;
            break;

        case MUIA_Version:
            *store = (IPTR)WANDERERVERS;
            break;

        case MUIA_Revision:
            *store = (IPTR)WANDERERREV;
            break;

        case MUIA_Application_Iconified:
            /* Wanderer does not allow iconifying of itself */
            *store = (IPTR)FALSE;
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

D(bug("[Wanderer] %s() ##\n[Wanderer] %s: Creating 'Workbench' Window..\n", __PRETTY_FUNCTION__, __PRETTY_FUNCTION__));

    data->wd_WorkbenchWindow = (Object *) DoMethod
    (
        self, MUIM_Wanderer_CreateDrawerWindow, (IPTR) NULL
    );

    if (data->wd_WorkbenchWindow != NULL)
    {
D(bug("[Wanderer] %s: Workbench Window Obj @ %x\n", __PRETTY_FUNCTION__, data->wd_WorkbenchWindow));

        Detach();

#ifdef __AROS__
D(bug("[Wanderer] %s: Really handing control to Zune ..\n", __PRETTY_FUNCTION__));

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
    
    /* TODO: Report an error if we fail to create the Workbench's window ... */
    
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

    D(bug("[Wanderer] Timer event, user screen title %s\n", scr_title));

    while ((child = NextObject(&cstate)))
    {
        /* Set the Wanderer title to each window's screen title */
        STRPTR current_title = NULL;

        GET(child, MUIA_Window_ScreenTitle, &current_title);
        D(bug("[Wanderer] Current title %s\n", current_title));

        if ((current_title == NULL) || (strcmp(current_title, scr_title) != 0))
	{
	    /* Limit the menu bar flickering */
            SET(child, MUIA_Window_ScreenTitle, (IPTR) scr_title);
	}

        /* Request rate-limited, conditional refresh */
        DoMethod(child, MUIM_IconWindow_RateLimitRefresh);
    }

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
D(bug("[Wanderer] %s: Received signal at notification port\n", __PRETTY_FUNCTION__));

    while ((wbhm = WBHM(GetMsg(data->wd_CommandPort))) != NULL)
    {
D(bug("[Wanderer] %s: Received message from handler, type = %ld\n", __PRETTY_FUNCTION__, wbhm->wbhm_Type));

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
/* TODO: We need to handle the possiblity that we fail to lock the pubscreen... */
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
                    BOOL         windowned = FALSE, refreshroot = FALSE;

                    switch (wbhm->wbhm_Data.Update.Type)
                    {
                        case WBDISK:
                        case WBDRAWER:
                        case WBGARBAGE:
                            length = strlen(name);
                            windowned = TRUE;
                            break;
                        case WBAPPICON:
                            refreshroot = TRUE;
                            break;
                        default:
                            length = PathPart(name) - name;
                            break;
                    }

D(bug("[Wanderer] %s: name = %s, length = %ld\n", __PRETTY_FUNCTION__, name, length));

                    if (windowned)
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
                                        DoMethod(iconlist, MUIM_IconList_Update);
                                        DoMethod(iconlist, MUIM_IconList_Sort);
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    if (refreshroot)
                    {
                        Object * root = (Object *)XGET(self, MUIA_Wanderer_WorkbenchWindow);
                        if (root)
                        {
                            Object *iconlist = (Object *)XGET(root, MUIA_IconWindow_IconList);
                            if (iconlist != NULL)
                            {
                                DoMethod(iconlist, MUIM_IconList_Update);
                                DoMethod(iconlist, MUIM_IconList_Sort);
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
                        self, MUIM_Wanderer_CreateDrawerWindow, (IPTR) buf
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
    struct Wanderer_FSHandler *nodeFSHandler;

    while ((plainMessage = GetMsg(data->wd_NotifyPort)) != NULL)
    {
        struct NotifyMessage *notifyMessage = (struct NotifyMessage *) plainMessage;
        IPTR                  notifyMessage_UserData = notifyMessage->nm_NReq->nr_UserData;
D(bug("[Wanderer] %s: got FS notification ('%s' @ 0x%p) userdata = 0x%p!\n", __PRETTY_FUNCTION__, notifyMessage->nm_NReq->nr_Name, notifyMessage, notifyMessage_UserData));

        if (notifyMessage_UserData != (IPTR)NULL)
        {
            /* Only IconWindowDrawerList, IconWindowVolumeList class at the moment */
            D(bug("[Wanderer] %s: Icon Window contents changed .. Updating\n", __PRETTY_FUNCTION__));
            nodeFSHandler = (struct Wanderer_FSHandler *)notifyMessage_UserData;
            nodeFSHandler->HandleFSUpdate(nodeFSHandler->target, notifyMessage);
            continue;
        }

        ForeachNode(&_WandererIntern_FSHandlerList, nodeFSHandler)
        {
            if ((notifyMessage_UserData == (IPTR)NULL) && (strcmp(notifyMessage->nm_NReq->nr_Name, nodeFSHandler->fshn_Node.ln_Name) == 0))
            {
D(bug("[Wanderer] %s: Notification for special handler for '%s'\n", __PRETTY_FUNCTION__, notifyMessage->nm_NReq->nr_Name));

                nodeFSHandler->HandleFSUpdate(self, notifyMessage);
            }
        }
        ReplyMsg((struct Message *)notifyMessage);
    }

    return 0;
}
///

///Wanderer__Func_CreateWandererIntuitionMenu()
/* Some differences here between volumes and subwindows */
Object * Wanderer__Func_CreateWandererIntuitionMenu( BOOL isRoot, BOOL isBackdrop)
{
    Object   *_NewWandIntMenu__menustrip = NULL;
    IPTR     _NewWandIntMenu__OPTION_BACKDROP = CHECKIT|MENUTOGGLE;
    IPTR     _NewWandIntMenu__OPTION_SHOWALL  = CHECKIT|MENUTOGGLE;

    if (isBackdrop)
    {
        _NewWandIntMenu__OPTION_BACKDROP |= CHECKED;
    }

    if ( isRoot )
    {
        struct NewMenu nm[] = {
            {NM_TITLE,          _(MSG_MEN_WANDERER) },
                {NM_ITEM,       _(MSG_MEN_BACKDROP),    _(MSG_MEN_SC_BACKDROP)  , _NewWandIntMenu__OPTION_BACKDROP      , 0, (APTR) MEN_WANDERER_BACKDROP },
                {NM_ITEM,       _(MSG_MEN_EXECUTE),     _(MSG_MEN_SC_EXECUTE)   , 0                                     , 0, (APTR) MEN_WANDERER_EXECUTE },
                {NM_ITEM,       _(MSG_MEN_SHELL),       _(MSG_MEN_SC_SHELL)     , 0                                     , 0, (APTR) MEN_WANDERER_SHELL },
#if defined(__AROS__)
                {NM_ITEM,       "AROS" },
                    {NM_SUB,    _(MSG_MEN_ABOUT),       NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_AROS_ABOUT },
                    {NM_SUB,    _(MSG_MEN_GUISET),      NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_AROS_GUISETTINGS },
#endif
                {NM_ITEM,       _(MSG_MEN_ABOUT),       NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_ABOUT },
                {NM_ITEM,       _(MSG_MEN_QUIT) ,       _(MSG_MEN_SC_QUIT)      , 0                                     , 0, (APTR) MEN_WANDERER_QUIT },
                {NM_ITEM,       _(MSG_MEN_SHUTDOWN),    NULL                    , 0                                        , 0, (APTR) MEN_WANDERER_SHUTDOWN },
                {NM_TITLE,      _(MSG_MEN_WINDOW),      NULL                    , 0 },
                {NM_ITEM,       _(MSG_MEN_UPDATE),      NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_UPDATE },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_CONTENTS),    _(MSG_MEN_SC_CONTENTS)  , 0                                     , 0, (APTR) MEN_WINDOW_SELECT },
                {NM_ITEM,       _(MSG_MEN_CLRSEL),      _(MSG_MEN_SC_CLRSEL)    , 0                                     , 0, (APTR) MEN_WINDOW_CLEAR },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_SNAPSHT) },
                    {NM_SUB,    _(MSG_MEN_WINDOW),      NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_SNAP_WIN },
                    {NM_SUB,    _(MSG_MEN_ALL),         NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_SNAP_ALL },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_VIEW) },
                    {NM_SUB,    _(MSG_MEN_ICVIEW),      NULL                    , CHECKIT|CHECKED                       , ~((1 << 0)|(1 << 3)), (APTR) MEN_WINDOW_VIEW_ICON },
                    {NM_SUB,    _(MSG_MEN_DCVIEW),      NULL                    , CHECKIT|NM_ITEMDISABLED               , ~((1 << 1)|(1 << 3)), (APTR) MEN_WINDOW_VIEW_DETAIL },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_ALLFIL),      NULL                    , _NewWandIntMenu__OPTION_SHOWALL|NM_ITEMDISABLED, 0, (APTR) MEN_WINDOW_VIEW_ALL },
                {NM_ITEM,       _(MSG_MEN_SORTIC) },
                    {NM_SUB,    _(MSG_MEN_CLNUP),       _(MSG_MEN_SC_CLNUP)     , 0                                     , 0, (APTR) MEN_WINDOW_SORT_NOW },
                    {NM_SUB,    _(MSG_MEN_ICONSORTING), NULL                    , CHECKIT|MENUTOGGLE|CHECKED            , 0, (APTR) MEN_WINDOW_SORT_ENABLE },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_BYNAME),      NULL                    , CHECKIT                               , ~((1 << 3)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_NAME },
                    {NM_SUB,    _(MSG_MEN_BYDATE),      NULL                    , CHECKIT|NM_ITEMDISABLED               , ~((1 << 4)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_DATE },
                    {NM_SUB,    _(MSG_MEN_BYSIZE),      NULL                    , CHECKIT|NM_ITEMDISABLED               , ~((1 << 5)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_SIZE },
                    {NM_SUB,    _(MSG_MEN_BYTYPE),      NULL                    , CHECKIT|NM_ITEMDISABLED               , ~((1 << 6)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_TYPE },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_REVERSE),     NULL                    , CHECKIT|MENUTOGGLE                    , 0, (APTR) MEN_WINDOW_SORT_REVERSE },
                    {NM_SUB,    _(MSG_MEN_DRWFRST),     NULL                    , CHECKIT|MENUTOGGLE|NM_ITEMDISABLED    , 0, (APTR) MEN_WINDOW_SORT_TOPDRAWERS },
                    {NM_SUB,    _(MSG_MEN_GROUPICONS),  NULL                    , CHECKIT|MENUTOGGLE|NM_ITEMDISABLED    , 0, (APTR) MEN_WINDOW_SORT_GROUP },
            {NM_TITLE,          _(MSG_MEN_ICON),        NULL                    , 0 },
                {NM_ITEM,       _(MSG_MEN_OPEN),        _(MSG_MEN_SC_OPEN)      , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_OPEN },
                {NM_ITEM,       _(MSG_MEN_RENAME),      _(MSG_MEN_SC_RENAME)    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_RENAME },
                {NM_ITEM,       _(MSG_MEN_INFO),        _(MSG_MEN_SC_INFO)      , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_INFORMATION },
                {NM_ITEM,       _(MSG_MEN_SNAPSHOT),    _(MSG_MEN_SC_SNAPSHOT)  , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_SNAPSHOT },
                {NM_ITEM,       _(MSG_MEN_UNSNAPSHOT),  _(MSG_MEN_SC_UNSNAPSHOT), NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_UNSNAPSHOT },
                {NM_ITEM,       _(MSG_MEN_LEAVE_OUT),   _(MSG_MEN_SC_LEAVE_OUT) , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_LEAVEOUT },
                {NM_ITEM,       _(MSG_MEN_PUT_AWAY),    _(MSG_MEN_SC_PUT_AWAY)  , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_PUTAWAY },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_DELETE),      NULL                    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_DELETE },
                {NM_ITEM,       _(MSG_MEN_FORMAT),      NULL                    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_FORMAT },
                {NM_ITEM,       _(MSG_EMPTY_TRASH),     NULL                    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_EMPTYTRASH},
            {NM_TITLE,          _(MSG_MEN_TOOLS),       NULL                    , NM_MENUDISABLED },
            {NM_END}
        };
        _NewWandIntMenu__menustrip = MUI_MakeObject(MUIO_MenustripNM, nm, (IPTR) NULL);
    }
    else
    {
        struct NewMenu nm[] = {
            {NM_TITLE,          _(MSG_MEN_WANDERER) },
                {NM_ITEM,       _(MSG_MEN_BACKDROP),    _(MSG_MEN_SC_BACKDROP)  , _NewWandIntMenu__OPTION_BACKDROP      , 0, (APTR) MEN_WANDERER_BACKDROP },
                {NM_ITEM,       _(MSG_MEN_EXECUTE),     _(MSG_MEN_SC_EXECUTE)   , 0                                     , 0, (APTR) MEN_WANDERER_EXECUTE },
                {NM_ITEM,       _(MSG_MEN_SHELL),       _(MSG_MEN_SC_SHELL)     , 0                                     , 0, (APTR) MEN_WANDERER_SHELL },
#if defined(__AROS__)
                {NM_ITEM,       "AROS" },
                    {NM_SUB,    _(MSG_MEN_ABOUT),       NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_AROS_ABOUT },
                    {NM_SUB,    _(MSG_MEN_GUISET),      NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_AROS_GUISETTINGS },
#endif
                {NM_ITEM,       _(MSG_MEN_ABOUT),       NULL                    , 0                                     , 0, (APTR) MEN_WANDERER_ABOUT },
                {NM_ITEM,       _(MSG_MEN_QUIT) ,       _(MSG_MEN_SC_QUIT)      , 0                                     , 0, (APTR) MEN_WANDERER_QUIT },
                {NM_ITEM,       _(MSG_MEN_SHUTDOWN),    NULL                        , 0                                        , 0, (APTR) MEN_WANDERER_SHUTDOWN },
            {NM_TITLE,          _(MSG_MEN_WINDOW),      NULL                    , 0 },
                {NM_ITEM,       _(MSG_MEN_NEWDRAW),     _(MSG_MEN_SC_NEWDRAW)   , 0                                     , 0, (APTR) MEN_WINDOW_NEW_DRAWER },
                {NM_ITEM,       _(MSG_MEN_OPENPAR),     NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_OPEN_PARENT },
                {NM_ITEM,       _(MSG_MEN_CLOSE),       _(MSG_MEN_SC_CLOSE)     , 0                                     , 0, (APTR) MEN_WINDOW_CLOSE },
                {NM_ITEM,       _(MSG_MEN_UPDATE),      NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_UPDATE },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_CONTENTS),    _(MSG_MEN_SC_CONTENTS)  , 0                                     , 0, (APTR) MEN_WINDOW_SELECT },
                {NM_ITEM,       _(MSG_MEN_CLRSEL),      _(MSG_MEN_SC_CLRSEL)    , 0                                     , 0, (APTR) MEN_WINDOW_CLEAR },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_SNAPSHT) },
                    {NM_SUB,    _(MSG_MEN_WINDOW),      NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_SNAP_WIN },
                    {NM_SUB,    _(MSG_MEN_ALL),         NULL                    , 0                                     , 0, (APTR) MEN_WINDOW_SNAP_ALL },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_VIEW)},
                    {NM_SUB,    _(MSG_MEN_ICVIEW),      NULL                    , CHECKIT|CHECKED                       ,~((1 << 0)|(1 << 3)), (APTR) MEN_WINDOW_VIEW_ICON },
                    {NM_SUB,    _(MSG_MEN_DCVIEW),      NULL                    , CHECKIT                                       ,~((1 << 1)|(1 << 3)), (APTR) MEN_WINDOW_VIEW_DETAIL },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_ALLFIL),      NULL                    , _NewWandIntMenu__OPTION_SHOWALL       , 0, (APTR) MEN_WINDOW_VIEW_ALL },
                {NM_ITEM,       _(MSG_MEN_SORTIC)},
                    {NM_SUB,    _(MSG_MEN_CLNUP),       _(MSG_MEN_SC_CLNUP)     , 0                                     , 0, (APTR) MEN_WINDOW_SORT_NOW },
                    {NM_SUB,    _(MSG_MEN_ICONSORTING), NULL                    , CHECKIT|MENUTOGGLE|CHECKED            , 0, (APTR) MEN_WINDOW_SORT_ENABLE },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_BYNAME),      NULL                    , CHECKIT                               , ~((1 << 3)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_NAME },
                    {NM_SUB,    _(MSG_MEN_BYDATE),      NULL                    , CHECKIT                               , ~((1 << 4)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_DATE },
                    {NM_SUB,    _(MSG_MEN_BYSIZE),      NULL                    , CHECKIT                               , ~((1 << 5)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_SIZE },
                    {NM_SUB,    _(MSG_MEN_BYTYPE),      NULL                    , CHECKIT|NM_ITEMDISABLED               , ~((1 << 6)|(1 << 1)|(1 << 8)|(1 << 9)|(1 << 10)), (APTR) MEN_WINDOW_SORT_TYPE },
                    {NM_SUB,    NM_BARLABEL },
                    {NM_SUB,    _(MSG_MEN_REVERSE),     NULL                    , CHECKIT|MENUTOGGLE                    , 0, (APTR) MEN_WINDOW_SORT_REVERSE },
                    {NM_SUB,    _(MSG_MEN_DRWFRST),     NULL                    , CHECKIT|MENUTOGGLE|CHECKED            , 0, (APTR) MEN_WINDOW_SORT_TOPDRAWERS },
                    {NM_SUB,    _(MSG_MEN_GROUPICONS),  NULL                    , CHECKIT|MENUTOGGLE|NM_ITEMDISABLED    , 0, (APTR) MEN_WINDOW_SORT_GROUP },
            {NM_TITLE,          _(MSG_MEN_ICON),        NULL                    , 0 },
                {NM_ITEM,       _(MSG_MEN_OPEN),        _(MSG_MEN_SC_OPEN)      , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_OPEN },
                {NM_ITEM,       _(MSG_MEN_RENAME),      _(MSG_MEN_SC_RENAME)    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_RENAME },
                {NM_ITEM,       _(MSG_MEN_INFO),        _(MSG_MEN_SC_INFO)      , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_INFORMATION },
                {NM_ITEM,       _(MSG_MEN_SNAPSHOT),    _(MSG_MEN_SC_SNAPSHOT)  , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_SNAPSHOT },
                {NM_ITEM,       _(MSG_MEN_UNSNAPSHOT),  _(MSG_MEN_SC_UNSNAPSHOT), NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_UNSNAPSHOT },
                {NM_ITEM,       _(MSG_MEN_LEAVE_OUT),   _(MSG_MEN_SC_LEAVE_OUT) , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_LEAVEOUT },
                {NM_ITEM,       _(MSG_MEN_PUT_AWAY),    _(MSG_MEN_SC_PUT_AWAY)  , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_PUTAWAY },
                {NM_ITEM,       NM_BARLABEL },
                {NM_ITEM,       _(MSG_MEN_DELETE),      NULL                    , NM_ITEMDISABLED                       , 0, (APTR) MEN_ICON_DELETE },
                {NM_ITEM,       _(MSG_MEN_FORMAT),      NULL                    , NM_ITEMDISABLED },
                {NM_ITEM,       _(MSG_EMPTY_TRASH),     NULL                    , NM_ITEMDISABLED },
            {NM_TITLE,          _(MSG_MEN_TOOLS),       NULL                    , NM_MENUDISABLED },
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
    IPTR    TAG_IconWindow_Drawer;
    Object *_NewWandDrawerMenu__menustrip;

    Object *window_IconList = NULL;

D(bug("[Wanderer]: %s()\n", __PRETTY_FUNCTION__));

    if ((isWorkbenchWindow = (message->drawer == NULL ? TRUE : FALSE)))
    {
        useBackdrop = data->wd_Option_BackDropMode;
    }

    TAG_IconWindow_Drawer = isWorkbenchWindow ? TAG_IGNORE : MUIA_IconWindow_Location;

    data->wd_Screen = LockPubScreen(NULL);

    if(data->wd_Screen == NULL)
    {
D(bug("[Wanderer] %s: Couldn't lock screen!\n", __PRETTY_FUNCTION__));
        CoerceMethod(CLASS, self, OM_DISPOSE);
        return NULL;
    }
D(bug("[Wanderer] %s: Using Screen @ %p\n", __PRETTY_FUNCTION__, data->wd_Screen));

    _NewWandDrawerMenu__menustrip = Wanderer__Func_CreateWandererIntuitionMenu (isWorkbenchWindow, useBackdrop);

    /* Create a new icon drawer window with the correct drawer being set */
#ifdef __AROS__ 
    window = (Object *)IconWindowObject,
                MUIA_UserData,                      1,
                MUIA_Wanderer_Prefs,                  (IPTR)data->wd_Prefs,
                MUIA_Wanderer_Screen,                 (IPTR)data->wd_Screen,
                MUIA_Window_ScreenTitle,              (IPTR)GetUserScreenTitle(data->wd_Prefs),
                MUIA_Window_Menustrip,                (IPTR) _NewWandDrawerMenu__menustrip,
                TAG_IconWindow_Drawer,                (IPTR) message->drawer,
                MUIA_IconWindow_ActionHook,           (IPTR) &_WandererIntern_hook_action,
                MUIA_IconWindow_IsRoot,               isWorkbenchWindow ? TRUE : FALSE,
                isWorkbenchWindow ? MUIA_IconWindow_IsBackdrop : TAG_IGNORE, useBackdrop,
                MUIA_Wanderer_FileSysNotifyPort, (IPTR)data->wd_NotifyPort,
                isWorkbenchWindow ? TAG_IGNORE : MUIA_Wanderer_FileSysNotifyList, (IPTR)&_WandererIntern_FSHandlerList,
                MUIA_Window_IsSubWindow,              isWorkbenchWindow ? FALSE : TRUE,
             End;
#else
     window = NewObject(IconWindow_CLASS->mcc_Class, NULL,
                MUIA_UserData,                      1,
                MUIA_Wanderer_Prefs,                  data->wd_Prefs,
                MUIA_Wanderer_Screen,                 data->wd_Screen,
                MUIA_Window_ScreenTitle,              GetUserScreenTitle(data->wd_Prefs),
                MUIA_Window_Menustrip,                (IPTR) _NewWandDrawerMenu__menustrip,
                TAG_IconWindow_Drawer,                (IPTR) message->drawer,
                MUIA_IconWindow_ActionHook,           (IPTR) &_WandererIntern_hook_action,
                MUIA_IconWindow_IsRoot,               isWorkbenchWindow ? TRUE : FALSE,
                isWorkbenchWindow ? MUIA_IconWindow_IsBackdrop : TAG_IGNORE, useBackdrop,
                MUIA_Wanderer_FileSysNotifyPort, data->wd_NotifyPort,
                isWorkbenchWindow ? TAG_IGNORE : MUIA_Wanderer_FileSysNotifyList, (IPTR)&_WandererIntern_FSHandlerList,
                MUIA_Window_IsSubWindow,              isWorkbenchWindow ? FALSE : TRUE,
             TAG_DONE);
#endif

    if (data->wd_Screen)
    {
D(bug("[Wanderer] %s: Unlocking access to screen @ %p\n", __PRETTY_FUNCTION__, data->wd_Screen));
        UnlockPubScreen(NULL, data->wd_Screen);
    }

    if (window != NULL)
    {
D(bug("[Wanderer] %s: window != NULL\n", __PRETTY_FUNCTION__));
        /* Get the drawer path back so we can use it also outside this function */
        STRPTR drw = NULL;
        BOOL freeDrwStr = FALSE;

        if (!isWorkbenchWindow) drw = (STRPTR) XGET(window, MUIA_IconWindow_Location);
        else
        {
D(bug("[Wanderer] %s: call AllocVec()\n", __PRETTY_FUNCTION__));
            drw = AllocVec ( 5, MEMF_CLEAR );
            sprintf ( drw, "RAM:" );
            freeDrwStr = TRUE;    
        }

        if (isWorkbenchWindow)
        {
D(bug("[Wanderer] %s: isWorkbenchWindow\n", __PRETTY_FUNCTION__));
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

D(bug("[Wanderer] %s: call get with MUIA_IconWindow_IconList\n", __PRETTY_FUNCTION__));
        GET(window, MUIA_IconWindow_IconList, &window_IconList);

D(bug("[Wanderer] %s: IconWindows IconList @ %p\n", __PRETTY_FUNCTION__, window_IconList));

        if (window_IconList != NULL)
        {
            struct Hook          *_wand_UpdateMenuStates_hook = NULL;

            if ((_wand_UpdateMenuStates_hook = AllocMem(sizeof(struct Hook), MEMF_CLEAR|MEMF_PUBLIC)) != NULL)
            {
                _wand_UpdateMenuStates_hook->h_Entry = ( HOOKFUNC )Wanderer__HookFunc_UpdateMenuStatesFunc;
                DoMethod
                  (
                    window_IconList, MUIM_Notify, MUIA_IconList_SelectionChanged, MUIV_EveryTime,
                    (IPTR) self, 3, 
                    MUIM_CallHook, _wand_UpdateMenuStates_hook, (IPTR)window
                  );
            }
            Wanderer__Func_UpdateMenuStates(window, window_IconList);
        }
D(bug("[Wanderer] %s: setup notifications\n", __PRETTY_FUNCTION__));
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
D(bug("[Wanderer] %s: execute all notifies\n", __PRETTY_FUNCTION__));
        /* If "Execute Command" entry is clicked open the execute window */
        SetMenuDefaultNotifies(self, _NewWandDrawerMenu__menustrip, drw);        

D(bug("[Wanderer] %s: add window to app\n", __PRETTY_FUNCTION__));
        /* Add the window to the application */
#ifdef __AROS__
        DoMethod(_app(self), OM_ADDMEMBER, (IPTR) window);
#else
        DoMethod(self, OM_ADDMEMBER, (IPTR) window);
#endif
D(bug("[Wanderer] %s: open window\n", __PRETTY_FUNCTION__));
        /* And now open it */
        DoMethod(window, MUIM_IconWindow_Open);
D(bug("[Wanderer] %s: clean up memory\n", __PRETTY_FUNCTION__));
        /* Clean up ram string */
        if ( freeDrwStr && drw ) FreeVec ( drw );
    }
D(bug("[Wanderer] %s: exit\n", __PRETTY_FUNCTION__));
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
