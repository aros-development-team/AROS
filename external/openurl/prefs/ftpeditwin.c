/***************************************************************************

 openurl.library - universal URL display and browser launcher library
 Copyright (C) 1998-2005 by Troels Walsted Hansen, et al.
 Copyright (C) 2005-2013 by openurl.library Open Source Team

 This library is free software; it has been placed in the public domain
 and you can freely redistribute it and/or modify it. Please note, however,
 that some components may be under the LGPL or GPL license.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

 openurl.library project: http://sourceforge.net/projects/openurllib/

 $Id$

***************************************************************************/

#include "openurl.h"

#define CATCOMP_NUMBERS
#include "locale.h"

#include <libraries/openurl.h>

#include "SDI_hook.h"
#include "macros.h"

#include "debug.h"

/**************************************************************************/

struct data
{
    Object                 *FTPList;

    Object                 *name;
    Object                 *path;
    Object                 *port;
    Object                 *removeScheme;

    Object                 *show;
    Object                 *toFront;
    Object                 *openURL;
    Object                 *openURLNW;

    Object                 *use;
    Object                 *cancel;

    struct URL_FTPNode     *fn;

    ULONG                  flags;
};

enum
{
    FLG_Notifies = 1<<0,
};

/**************************************************************************/

static CONST_STRPTR syms[] =
{
    "%u",
    "%p",
    NULL
};

static STRPTR names[] =
{
    (STRPTR)MSG_Edit_PopURL,
    (STRPTR)MSG_Edit_PopScreen,
    NULL
};

static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct data        temp;
    struct URL_FTPNode *fn;
    struct TagItem     *attrs = msg->ops_AttrList;

    memset(&temp,0,sizeof(temp));

    temp.FTPList = (Object *)GetTagData(MUIA_FTPEditWin_ListObj,(IPTR)NULL,attrs);
    if (!temp.FTPList) return 0;

    fn = temp.fn  = (struct URL_FTPNode *)GetTagData(MUIA_FTPEditWin_FTP,(IPTR)NULL,attrs);
    if (!fn) return 0;


    if((obj = (Object *)DoSuperNew(cl,obj,
        MUIA_HelpNode,             "FWIN",
        MUIA_Window_ID,            MAKE_ID('E','D','B','R'),
        MUIA_Window_Title,         getString(MSG_FTP_WinTitle),
        MUIA_Window_AllowTopMenus, FALSE,
        MUIA_Window_ScreenTitle,   getString(MSG_App_ScreenTitle),

        WindowContents, VGroup,
            Child, ColGroup(2),
                GroupFrameT(getString(MSG_Edit_Definitions)),

                Child, olabel2(MSG_Edit_Name),
                Child, temp.name = ostring(NAME_LEN,MSG_Edit_Name,MSG_Edit_Name_Help),

                Child, olabel2(MSG_Edit_Path),
                Child, temp.path = opopph(syms,names,PATH_LEN,MSG_Edit_Path,TRUE,MSG_Edit_Path_Help),

                Child, olabel2(MSG_Edit_Port),
                Child, temp.port = opopport(PORT_LEN,MSG_Edit_Port,MSG_Edit_Port_Help),

                Child, olabel2(MSG_FTP_RemoveURLQualifier),
                Child, HGroup,
                    Child, temp.removeScheme = ocheckmark(MSG_FTP_RemoveURLQualifier,MSG_FTP_RemoveURLQualifier_Help),
                    Child, HSpace(0),
                End,
            End,

            Child, ColGroup(2),
                GroupFrameT(getString(MSG_Edit_ARexx)),

                Child, olabel2(MSG_Edit_Show),
                Child, temp.show = ostring(SHOWCMD_LEN,MSG_Edit_Show,MSG_Edit_Show_Help),

                Child, olabel2(MSG_Edit_Screen),
                Child, temp.toFront = ostring(TOFRONTCMD_LEN,MSG_Edit_Screen,MSG_Edit_Screen_Help),

                Child, olabel2(MSG_Edit_OpenURL),
                Child, temp.openURL = opopph(syms,names,OPENURLCMD_LEN,MSG_Edit_OpenURL,FALSE,MSG_Edit_OpenURL_Help),
                Child, olabel2(MSG_Edit_NewWin),
                Child, temp.openURLNW = opopph(syms,names,OPENURLWCMD_LEN,MSG_Edit_NewWin,FALSE,MSG_Edit_NewWin_Help),
            End,

            Child, ColGroup(3),
                Child, temp.use = obutton(MSG_Edit_Use,MSG_Edit_Use_Help),
                Child, RectangleObject, End,
                Child, temp.cancel = obutton(MSG_Edit_Cancel,MSG_Edit_Cancel_Help),
            End,
        End,
        TAG_MORE, attrs)) != NULL)
    {
        struct data *data = INST_DATA(cl,obj);

        CopyMem(&temp,data,sizeof(*data));

        set(data->name, MUIA_String_Contents, fn->ufn_Name);
        set(data->path, MUIA_String_Contents, fn->ufn_Path);
        set(data->port, MUIA_String_Contents, fn->ufn_Port);
        set(data->removeScheme, MUIA_Selected, isFlagSet(fn->ufn_Flags, UFNF_REMOVEFTP));
        set(data->show, MUIA_String_Contents, fn->ufn_ShowCmd);
        set(data->toFront, MUIA_String_Contents, fn->ufn_ToFrontCmd);
        set(data->openURL, MUIA_String_Contents, fn->ufn_OpenURLCmd);
        set(data->openURLNW, MUIA_String_Contents, fn->ufn_OpenURLWCmd);
    }

    return (IPTR)obj;
}

/**************************************************************************/

static IPTR mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct data *data = INST_DATA(cl,obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_FTPEditWin_FTP: *msg->opg_Storage = (IPTR)data->fn; return TRUE;
        case MUIA_App_IsSubWin:   *msg->opg_Storage = TRUE; return TRUE;
        default: return DoSuperMethodA(cl,obj,(Msg)msg);
    }
}

/**************************************************************************/

static IPTR mWindow_Setup(struct IClass *cl, Object *obj, struct MUIP_Window_Setup *msg)
{
  IPTR result = FALSE;

  ENTER();

  if(DoSuperMethodA(cl, obj, (Msg)msg))
  {
    struct data *data = INST_DATA(cl, obj);

    if(isFlagClear(data->flags, FLG_Notifies))
    {
      DoMethod(data->use, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1, MUIM_FTPEditWin_Use);
      DoMethod(data->cancel, MUIM_Notify, MUIA_Pressed, FALSE,(IPTR)obj, 3, MUIM_Set, MUIA_Window_CloseRequest, TRUE);

      DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest,TRUE, (IPTR)_app(obj), 6, MUIM_Application_PushMethod,
          (IPTR)_app(obj), 3, MUIM_App_CloseWin, MUIA_FTPEditWin_FTP, (IPTR)data->fn);

      SET_FLAG(data->flags, FLG_Notifies);

      result = TRUE;
    }
  }

  RETURN(result);
  return result;
}

/**************************************************************************/

static IPTR mUse(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data        *data = INST_DATA(cl,obj);
    struct URL_FTPNode *fn = data->fn;
    LONG               i, visible, first;

    CLEAR_FLAG(fn->ufn_Flags, UNF_NEW);

    strlcpy(fn->ufn_Name, (STRPTR)xget(data->name,MUIA_String_Contents), sizeof(fn->ufn_Name));
    strlcpy(fn->ufn_Path, (STRPTR)xget(data->path,MUIA_String_Contents), sizeof(fn->ufn_Path));
    strlcpy(fn->ufn_Port, (STRPTR)xget(data->port,MUIA_String_Contents), sizeof(fn->ufn_Port));
    if (xget(data->removeScheme,MUIA_Selected))
      SET_FLAG(fn->ufn_Flags, UFNF_REMOVEFTP);
    else
      CLEAR_FLAG(fn->ufn_Flags, UFNF_REMOVEFTP);

    strlcpy(fn->ufn_ShowCmd, (STRPTR)xget(data->show,MUIA_String_Contents), sizeof(fn->ufn_ShowCmd));
    strlcpy(fn->ufn_ToFrontCmd, (STRPTR)xget(data->toFront,MUIA_String_Contents), sizeof(fn->ufn_ToFrontCmd));
    strlcpy(fn->ufn_OpenURLCmd, (STRPTR)xget(data->openURL,MUIA_String_Contents), sizeof(fn->ufn_OpenURLCmd));
    strlcpy(fn->ufn_OpenURLWCmd, (STRPTR)xget(data->openURLNW,MUIA_String_Contents), sizeof(fn->ufn_OpenURLWCmd));

    visible = xget(data->FTPList, MUIA_List_Visible);
    if (visible!=-1)
    {
        first = xget(data->FTPList, MUIA_List_First);

        for (i = first; i < (first + visible); i++)
        {
            DoMethod(data->FTPList,MUIM_List_GetEntry,i,(IPTR)&fn);
            if (!fn) break;

            if (fn==data->fn)
            {
                DoMethod(data->FTPList,MUIM_List_Redraw,i);
                break;
            }
        }
    }

    set(obj,MUIA_Window_CloseRequest,TRUE);

    return TRUE;
}

/**************************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                 return mNew(cl,obj,(APTR)msg);
        case OM_GET:                 return mGet(cl,obj,(APTR)msg);

        case MUIM_Window_Setup:      return mWindow_Setup(cl,obj,(APTR)msg);

        case MUIM_FTPEditWin_Use:    return mUse(cl,obj,(APTR)msg);

        default:                     return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

BOOL initFTPEditWinClass(void)
{
    BOOL success = FALSE;

    if((g_FTPEditWinClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
    {
        localizeStrings(names);
        success = TRUE;
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposeFTPEditWinClass(void)
{
    ENTER();

    if(g_FTPEditWinClass != NULL)
        MUI_DeleteCustomClass(g_FTPEditWinClass);

    LEAVE();
}

/**************************************************************************/

