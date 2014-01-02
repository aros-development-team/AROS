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

    Object                 *browserList;

    Object                 *name;
    Object                 *path;
    Object                 *port;

    Object                 *show;
    Object                 *toFront;
    Object                 *openURL;
    Object                 *openURLNW;

    Object                 *use;
    Object                 *cancel;

    struct URL_BrowserNode *bn;

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
    struct data            temp;
    struct URL_BrowserNode *bn;
    struct TagItem         *attrs = msg->ops_AttrList;

    memset(&temp,0,sizeof(temp));

    temp.browserList  = (Object *)GetTagData(MUIA_BrowserEditWin_ListObj,(IPTR)NULL,attrs);
    if (!temp.browserList) return 0;

    bn = temp.bn = (struct URL_BrowserNode *)GetTagData(MUIA_BrowserEditWin_Browser,(IPTR)NULL,attrs);
    if (!bn) return 0;


    if((obj = (Object *)DoSuperNew(cl,obj,
        MUIA_HelpNode,             "BWIN",
        MUIA_Window_ID,            MAKE_ID('E','D','B','R'),
        MUIA_Window_Title,         getString(MSG_Browser_WinTitle),
        MUIA_Window_ScreenTitle,   getString(MSG_App_ScreenTitle),
        MUIA_Window_AllowTopMenus, FALSE,

        WindowContents, VGroup,
            Child, ColGroup(2),
                GroupFrameT(getString(MSG_Edit_Definitions)),

                Child, olabel2(MSG_Edit_Name),
                Child, temp.name = ostring(NAME_LEN,MSG_Edit_Name,MSG_Edit_Name_Help),

                Child, olabel2(MSG_Edit_Path),
                Child, temp.path = opopph(syms,names,PATH_LEN,MSG_Edit_Path,TRUE,MSG_Edit_Path_Help),
                Child, olabel2(MSG_Edit_Port),
                Child, temp.port = opopport(PORT_LEN,MSG_Edit_Port,MSG_Edit_Port_Help),
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

        set(data->name,MUIA_String_Contents,bn->ubn_Name);
        set(data->path,MUIA_String_Contents,bn->ubn_Path);
        set(data->port,MUIA_String_Contents,bn->ubn_Port);
        set(data->show,MUIA_String_Contents,bn->ubn_ShowCmd);
        set(data->toFront,MUIA_String_Contents,bn->ubn_ToFrontCmd);
        set(data->openURL,MUIA_String_Contents,bn->ubn_OpenURLCmd);
        set(data->openURLNW,MUIA_String_Contents,bn->ubn_OpenURLWCmd);
    }

    return (IPTR)obj;
}

/**************************************************************************/

static IPTR mGet(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct data *data = INST_DATA(cl,obj);

    switch (msg->opg_AttrID)
    {
        case MUIA_BrowserEditWin_Browser: *msg->opg_Storage = (IPTR)data->bn; return TRUE;
        case MUIA_App_IsSubWin:           *msg->opg_Storage = TRUE; return TRUE;
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
      DoMethod(data->use, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 1, MUIM_BrowserEditWin_Use);
      DoMethod(data->cancel, MUIM_Notify, MUIA_Pressed, FALSE, (IPTR)obj, 3, MUIM_Set, MUIA_Window_CloseRequest, TRUE);

      DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, (IPTR)_app(obj), 6, MUIM_Application_PushMethod,
          (IPTR)_app(obj), 3, MUIM_App_CloseWin, MUIA_BrowserEditWin_Browser, (IPTR)data->bn);

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
    struct data            *data = INST_DATA(cl,obj);
    struct URL_BrowserNode *bn = data->bn;
    LONG                   i, visible, first;

    CLEAR_FLAG(bn->ubn_Flags, UNF_NEW);

    strlcpy(bn->ubn_Name, (STRPTR)xget(data->name,MUIA_String_Contents), sizeof(bn->ubn_Name));
    strlcpy(bn->ubn_Path, (STRPTR)xget(data->path,MUIA_String_Contents), sizeof(bn->ubn_Path));
    strlcpy(bn->ubn_Port, (STRPTR)xget(data->port,MUIA_String_Contents), sizeof(bn->ubn_Port));

    strlcpy(bn->ubn_ShowCmd, (STRPTR)xget(data->show,MUIA_String_Contents), sizeof(bn->ubn_ShowCmd));
    strlcpy(bn->ubn_ToFrontCmd, (STRPTR)xget(data->toFront,MUIA_String_Contents), sizeof(bn->ubn_ToFrontCmd));
    strlcpy(bn->ubn_OpenURLCmd, (STRPTR)xget(data->openURL,MUIA_String_Contents), sizeof(bn->ubn_OpenURLCmd));
    strlcpy(bn->ubn_OpenURLWCmd, (STRPTR)xget(data->openURLNW,MUIA_String_Contents), sizeof(bn->ubn_OpenURLWCmd));

    visible = xget(data->browserList, MUIA_List_Visible);
    if (visible!=-1)
    {
        first = xget(data->browserList, MUIA_List_First);

        for (i = first; i < (first + visible); i++)
        {
            DoMethod(data->browserList,MUIM_List_GetEntry,i,(IPTR)&bn);
            if (!bn) break;

            if (bn==data->bn)
            {
                DoMethod(data->browserList,MUIM_List_Redraw,i);
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
        case OM_NEW:                  return mNew(cl,obj,(APTR)msg);
        case OM_GET:                  return mGet(cl,obj,(APTR)msg);

        case MUIM_Window_Setup:       return mWindow_Setup(cl,obj,(APTR)msg);

        case MUIM_BrowserEditWin_Use: return mUse(cl,obj,(APTR)msg);

        default:                      return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

BOOL initBrowserEditWinClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((g_browserEditWinClass = MUI_CreateCustomClass(NULL, MUIC_Window, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
    {
        localizeStrings(names);
        success = TRUE;
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposeBrowserEditWinClass(void)
{
    ENTER();

    if(g_browserEditWinClass != NULL)
        MUI_DeleteCustomClass(g_browserEditWinClass);

    LEAVE();
}

/**************************************************************************/

