/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2009 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include <proto/openurl.h>

#include "openurl.h"

#define CATCOMP_NUMBERS
#include "locale.h"

#include <libraries/openurl.h>

#include <SDI/SDI_hook.h>
#include "macros.h"

#include "debug.h"

/**************************************************************************/

struct data
{
    Object *reg;

    Object *info;
    Object *about;
    Object *browsers;
    Object *mailers;
    Object *FTPs;
    Object *misc;

    Object *browserList;
    Object *mailerList;
    Object *FTPList;

    Object *prepend;
    Object *mailto;
    Object *useFTP;

    Object *show;
    Object *toFront;
    Object *newWin;
    Object *launch;

    Object *save;
    Object *use;
    Object *apply;
    Object *cancel;

    ULONG  flags;
};

enum
{
    FLG_Notifies = 1<<0,
};

/**************************************************************************/

static STRPTR tabs[] =
{
    (STRPTR)MSG_Win_Labels_Browsers,
    (STRPTR)MSG_Win_Labels_Mailers,
    (STRPTR)MSG_Win_Labels_FTPs,
    (STRPTR)MSG_Win_Labels_Misc,
    NULL
};

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct data temp;

    memset(&temp,0,sizeof(temp));

    if((obj = (Object *)DoSuperNew(cl,obj,
        MUIA_HelpNode,           "WIN",
        MUIA_Window_ID,          MAKE_ID('M', 'W', 'I', 'N'),
        MUIA_Window_Title,       getString(MSG_Win_WinTitle),
        MUIA_Window_ScreenTitle, getString(MSG_App_ScreenTitle),

        WindowContents, VGroup,
            Child, temp.reg = RegisterObject,
                MUIA_Background,       MUII_RegisterBack,
                MUIA_CycleChain,       TRUE,
                MUIA_Register_Titles,  tabs,

                /* Browsers */
                Child, temp.browsers = appListObject,
                    MUIA_AppList_Type, MUIV_AppList_Type_Browser,
                End,

                /* Mailers */
                Child, temp.mailers = appListObject,
                    MUIA_AppList_Type, MUIV_AppList_Type_Mailer,
                End,

                /* FTPs */
                Child, temp.FTPs = appListObject,
                    MUIA_AppList_Type, MUIV_AppList_Type_FTP,
                End,

                /* Miscellaneous */
                Child, temp.misc = VGroup,
                    MUIA_HelpNode, "MISCS",

                    /* Defaults */
                    Child, VGroup,
                        GroupFrameT(getString(MSG_Misc_Defaults)),
                        Child, HGroup,
                            Child, ColGroup(2),
                                Child, temp.show = ocheckmark(MSG_Misc_Show,MSG_Misc_Show_Help),
                                Child, ollabel1(MSG_Misc_Show),
                                Child, temp.toFront = ocheckmark(MSG_Misc_Bring,MSG_Misc_Bring_Help),
                                Child, ollabel1(MSG_Misc_Bring),
                                Child, temp.newWin = ocheckmark(MSG_Misc_Open,MSG_Misc_Open_Help),
                                Child, ollabel1(MSG_Misc_Open),
                                Child, temp.launch = ocheckmark(MSG_Misc_Launch,MSG_Misc_Launch_Help),
                                Child, ollabel1(MSG_Misc_Launch),
                            End,
                            Child, HSpace(0),
                        End,
                        Child, VSpace(0),
                    End,

                    /* Options */
                    Child, VGroup,
                        GroupFrameT(getString(MSG_Misc_Options)),
                        Child, HGroup,
                            Child, ColGroup(2),
                                Child, temp.prepend = ocheckmark(MSG_Misc_Prepend,MSG_Misc_Prepend_Help),
                                Child, ollabel1(MSG_Misc_Prepend),
                                Child, temp.mailto = ocheckmark(MSG_Misc_UseMailer,MSG_Misc_UseMailer_Help),
                                Child, ollabel1(MSG_Misc_UseMailer),
                                Child, temp.useFTP = ocheckmark(MSG_Misc_UseFTP,MSG_Misc_UseFTP_Help),
                                Child, ollabel1(MSG_Misc_UseFTP),
                            End,
                            Child, HSpace(0),
                        End,
                        Child, VSpace(0),
                    End,
                End,
            End,
            /* Buttons */
            Child, HGroup,
                Child, temp.save = obutton(MSG_Win_Save,MSG_Win_Save_Help),
                Child, wspace(16),
                Child, temp.use = obutton(MSG_Win_Use,MSG_Win_Use_Help),
                Child, wspace(16),
                Child, temp.apply = obutton(MSG_Win_Apply,MSG_Win_Apply_Help),
                Child, wspace(16),
                Child, temp.cancel = obutton(MSG_Win_Cancel,MSG_Win_Cancel_Help),
            End,

        End,
        TAG_MORE, msg->ops_AttrList)) != NULL)
    {
        struct data *data = INST_DATA(cl,obj);

        /* init instance data */
        CopyMem(&temp,data,sizeof(*data));

        data->browserList = (Object *)xget(data->browsers, MUIA_AppList_ListObj);
        data->mailerList = (Object *)xget(data->mailers, MUIA_AppList_ListObj);
        data->FTPList = (Object *)xget(data->FTPs, MUIA_AppList_ListObj);

        /* buttons */
        set(obj,MUIA_Window_DefaultObject,data->browserList);

        /* window notifies */
        DoMethod(obj,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,MUIV_Notify_Application,2,
            MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

        /* buttons notifies */
        DoMethod(data->save,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Save);
        DoMethod(data->use,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Use);
        DoMethod(data->apply,MUIM_Notify,MUIA_Pressed,FALSE,(IPTR)obj,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Apply);
        DoMethod(data->cancel,MUIM_Notify,MUIA_Pressed,FALSE,MUIV_Notify_Application,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
    }

    return (IPTR)obj;
}

/**************************************************************************/

static IPTR mGetPrefs(struct IClass *cl,Object *obj, struct MUIP_Win_GetPrefs *msg)
{
    struct data            *data = INST_DATA(cl,obj);
    struct URL_Prefs       *p;
    struct URL_BrowserNode *bn;
    struct URL_MailerNode  *mn;
    struct URL_FTPNode     *fn;
    ULONG                  mode, error = 0;

    /* get the openurl.library prefs */

    switch (msg->mode)
    {
        case MUIV_Win_GetPrefs_InUse:      mode = URL_GetPrefs_Mode_InUse;   break;
        case MUIV_Win_GetPrefs_LastSaveds: mode = URL_GetPrefs_Mode_Envarc;  break;
        case MUIV_Win_GetPrefs_Restore:    mode = URL_GetPrefs_Mode_Env;     break;
        case MUIV_Win_GetPrefs_Defaults:   mode = URL_GetPrefs_Mode_Default; break;
        default: return FALSE;
    }

    p = URL_GetPrefs(URL_GetPrefs_Mode,mode,TAG_DONE);
    if (!p) error = MSG_Err_NoPrefs;
    else if (p->up_Version!=PREFS_VERSION) error = MSG_Err_BadPrefs;

    if (error)
    {
        MUI_Request(_app(obj),NULL,0,getString(MSG_ErrReqTitle),
                                     getString(MSG_ErrReqGadget),
                                     getString(error),
                                     p ? p->up_Version : 0);

        if (p) URL_FreePrefsA(p,NULL);

        return FALSE;
    }

    /* Browsers */
    set(data->browserList,MUIA_List_Quiet,TRUE);
    DoMethod(data->browserList,MUIM_List_Clear);

    for (bn = (struct URL_BrowserNode *)p->up_BrowserList.mlh_Head;
         bn->ubn_Node.mln_Succ;
         bn = (struct URL_BrowserNode *)bn->ubn_Node.mln_Succ)
    {
        DoMethod(data->browserList,MUIM_List_InsertSingle,(IPTR)bn,MUIV_List_Insert_Bottom);
    }

    set(data->browserList,MUIA_List_Quiet,FALSE);

    /* Mailers */
    set(data->mailerList,MUIA_List_Quiet,TRUE);
    DoMethod(data->mailerList,MUIM_List_Clear);

    for (mn = (struct URL_MailerNode *)p->up_MailerList.mlh_Head;
         mn->umn_Node.mln_Succ;
         mn = (struct URL_MailerNode *)mn->umn_Node.mln_Succ)
    {
        DoMethod(data->mailerList,MUIM_List_InsertSingle,(IPTR)mn,MUIV_List_Insert_Bottom);
    }

    set(data->mailerList,MUIA_List_Quiet,FALSE);

    /* FTPs */
    set(data->FTPList,MUIA_List_Quiet,TRUE);
    DoMethod(data->FTPList,MUIM_List_Clear);

    for (fn = (struct URL_FTPNode *)p->up_FTPList.mlh_Head;
         fn->ufn_Node.mln_Succ;
         fn = (struct URL_FTPNode *)fn->ufn_Node.mln_Succ)
    {
        DoMethod(data->FTPList,MUIM_List_InsertSingle,(IPTR)fn,MUIV_List_Insert_Bottom);
    }

    set(data->FTPList,MUIA_List_Quiet,FALSE);

    /* Miscellaneous */
    set(data->prepend, MUIA_Selected, isFlagSet(p->up_Flags, UPF_PREPENDHTTP));
    set(data->mailto, MUIA_Selected, isFlagSet(p->up_Flags, UPF_DOMAILTO));
    set(data->useFTP, MUIA_Selected, isFlagSet(p->up_Flags, UPF_DOFTP));

    set(data->show, MUIA_Selected, p->up_DefShow);
    set(data->toFront, MUIA_Selected, p->up_DefBringToFront);
    set(data->newWin, MUIA_Selected, p->up_DefNewWindow);
    set(data->launch, MUIA_Selected, p->up_DefLaunch);

    /* Activate the first entry */
    DoSuperMethod(cl,obj,MUIM_MultiSet,MUIA_List_Active,MUIV_List_Active_Top,
        (IPTR)data->browserList,
        (IPTR)data->mailerList,
        (IPTR)data->FTPList,
        NULL);

    URL_FreePrefsA(p,NULL);

    return TRUE;
}

/**************************************************************************/

static IPTR mStorePrefs(struct IClass *cl, Object *obj, struct MUIP_Win_StorePrefs *msg)
{
    struct data *data = INST_DATA(cl,obj);
    struct URL_Prefs     up;
    ULONG                i;

    set(_app(obj),MUIA_Application_Sleep,TRUE);

    /* Copy settings from gadgets into structure */
    up.up_Version = PREFS_VERSION;
    NewList((struct List *)&up.up_BrowserList);
    NewList((struct List *)&up.up_MailerList);
    NewList((struct List *)&up.up_FTPList);

    /* Browsers */
    for (i = 0; ; i++)
    {
        struct URL_BrowserNode *bn;

        DoMethod(data->browserList,MUIM_List_GetEntry,i,(IPTR)&bn);
        if (!bn) break;

        if(isFlagClear(bn->ubn_Flags, UNF_NEW))
            AddTail((struct List *)&up.up_BrowserList,(struct Node *)bn);
    }

    /* Mailers */
    for (i = 0; ; i++)
    {
        struct URL_MailerNode *mn;

        DoMethod(data->mailerList,MUIM_List_GetEntry,i,(IPTR)&mn);
        if (!mn) break;

        if(isFlagClear(mn->umn_Flags, UNF_NEW))
            AddTail((struct List *)&up.up_MailerList,(struct Node *)mn);
    }

    /* FTPs */
    for (i = 0; ; i++)
    {
        struct URL_FTPNode *fn;

        DoMethod(data->FTPList,MUIM_List_GetEntry,i,(IPTR)&fn);
        if (!fn) break;

        if(isFlagClear(fn->ufn_Flags, UNF_NEW))
            AddTail((struct List *)&up.up_FTPList,(struct Node *)fn);
    }

    /* Miscellaneous */
    if (xget(data->prepend,MUIA_Selected))
        SET_FLAG(up.up_Flags, UPF_PREPENDHTTP);
    else
        CLEAR_FLAG(up.up_Flags, UPF_PREPENDHTTP);

    if (xget(data->mailto,MUIA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOMAILTO);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOMAILTO);

    if (xget(data->useFTP,MUIA_Selected))
        SET_FLAG(up.up_Flags, UPF_DOFTP);
    else
        CLEAR_FLAG(up.up_Flags, UPF_DOFTP);

    up.up_DefShow         = (ULONG)xget(data->show,MUIA_Selected);
    up.up_DefBringToFront = (ULONG)xget(data->toFront,MUIA_Selected);
    up.up_DefNewWindow    = (ULONG)xget(data->newWin,MUIA_Selected);
    up.up_DefLaunch       = (ULONG)xget(data->launch,MUIA_Selected);

    /* Save to disk */
    if (!URL_SetPrefs(&up,URL_SetPrefs_Save,msg->How==MUIV_Win_StorePrefs_Save,TAG_DONE))
        MUI_RequestA(_app(obj),obj,0,getString(MSG_ErrReqTitle),getString(MSG_ErrReqGadget),getString(MSG_Err_FailedSave),NULL);

    set(_app(obj),MUIA_Application_Sleep,FALSE);

    /* Quit */
    if (msg->How!=MUIV_Win_StorePrefs_Apply)
        DoMethod(_app(obj),MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

    return TRUE;
}

/**************************************************************************/

static IPTR mDelete(struct IClass *cl, Object *obj, struct MUIP_Win_Delete *msg)
{
    struct data *data = INST_DATA(cl,obj);

    if (!delEntry(data->browserList,msg->entry))
        if (!delEntry(data->mailerList,msg->entry))
            delEntry(data->FTPList,msg->entry);

    return 0;
}

/**************************************************************************/

static IPTR mCheckSave(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    return (IPTR)(DoMethod(data->browsers,MUIM_App_CheckSave) ||
                   DoMethod(data->mailers,MUIM_App_CheckSave)  ||
                   DoMethod(data->FTPs,MUIM_App_CheckSave));
}

/**************************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:              return mNew(cl,obj,(APTR)msg);

        case MUIM_Win_GetPrefs:   return mGetPrefs(cl,obj,(APTR)msg);
        case MUIM_Win_StorePrefs: return mStorePrefs(cl,obj,(APTR)msg);
        case MUIM_Win_Delete:     return mDelete(cl,obj,(APTR)msg);
        case MUIM_App_CheckSave:  return mCheckSave(cl,obj,(APTR)msg);
		  //case 0x80426771:  return FALSE;

        default:                  return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

BOOL initWinClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((g_winClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
    {
        localizeStrings(tabs);
        success = TRUE;
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposeWinClass(void)
{
    ENTER();

    if(g_winClass != NULL)
        MUI_DeleteCustomClass(g_winClass);

    LEAVE();
}

/**************************************************************************/

