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
#include "version.h"

#include <libraries/openurl.h>

#include <SDI/SDI_hook.h>

#include "debug.h"

/**************************************************************************/

struct data
{
  Object            *win;
  Object            *about;

  struct DiskObject *icon;
};

/**************************************************************************/
/*
** Menus
*/

static struct NewMenu menu[] =
{
    MTITLE(MSG_Menu_Project),
        MITEM(MSG_Menu_About),
        MITEM(MSG_Menu_AboutMUI),
        MBAR,
        MITEM(MSG_Menu_Hide),
        MBAR,
        MITEM(MSG_Menu_Quit),

    MTITLE(MSG_Menu_Prefs),
        MITEM(MSG_Menu_Save),
        MITEM(MSG_Menu_Use),
        MBAR,
        MITEM(MSG_Menu_LastSaved),
        MITEM(MSG_Menu_Restore),
        MITEM(MSG_Menu_Defaults),
        MBAR,
        MITEM(MSG_Menu_MUI),

    MEND
};

/*
** Used classes
*/

static CONST_STRPTR usedClasses[] =
{
    "Urltext.mcc",
    NULL
};

/*
** Here we go
*/
static IPTR mNew(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *strip, *win;

    if((obj = (Object *)DoSuperNew(cl,obj,
            MUIA_Application_Title,       "OpenURL-Prefs",
            MUIA_Application_Version,     "$VER: OpenURL-Prefs " LIB_REV_STRING " [" SYSTEMSHORT "/" CPU "] (" LIB_DATE ") " LIB_COPYRIGHT,
            MUIA_Application_Author,      APPAUTHOR,
            MUIA_Application_Copyright,   getString(MSG_App_Copyright),
            MUIA_Application_Description, getString(MSG_App_Description),
            MUIA_Application_HelpFile,    APPHELP,
            MUIA_Application_Base,        APPBASENAME,
            MUIA_Application_Menustrip,   strip = MUI_MakeObject(MUIO_MenustripNM,(IPTR)menu,MUIO_MenustripNM_CommandKeyCheck),
            MUIA_Application_UsedClasses, usedClasses,
            MUIA_Application_Window,      win = winObject, End,
            TAG_MORE,msg->ops_AttrList)) != NULL)
    {
        struct data *data = INST_DATA(cl,obj);

        /*
        ** Setup data
        */

        data->win  = win;

        if((data->icon = GetDiskObject((STRPTR)"PROGDIR:OpenURL")) != NULL)
            superset(cl,obj,MUIA_Application_DiskObject,data->icon);

        /* Menus */

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_About),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,1,MUIM_App_About);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_AboutMUI),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)MUIV_Notify_Application,2,MUIM_Application_AboutMUI,(IPTR)win);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Hide),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,3,MUIM_Set,MUIA_Application_Iconified,TRUE);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Quit),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Save),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)win,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Save);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Use),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)win,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Use);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_LastSaved),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_LastSaveds);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Restore),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_Restore);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Defaults),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_Defaults);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_MUI),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(IPTR)obj,2,MUIM_Application_OpenConfigWindow,0);

        /* Menus help */
        DoSuperMethod(cl,obj,MUIM_Notify,MUIA_Application_MenuHelp,MUIV_EveryTime,MUIV_Notify_Self,
            5,MUIM_Application_ShowHelp,(IPTR)win,(IPTR)APPHELP,(IPTR)"MENUS",0);

        /*
        ** Load list formats
        */
        DoSuperMethod(cl,obj,MUIM_Application_Load,(IPTR)MUIV_Application_Load_ENV);

        /*
        ** Try to get OpenURL prefs and open window
        */
        if (!DoMethod(win,MUIM_Win_GetPrefs,MUIV_Win_GetPrefs_Restore) || !openWindow(obj,win))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);

            return 0;
        }
    }

    return (IPTR)obj;
}

/***********************************************************************/

static IPTR mDispose(struct IClass *cl, Object *obj, Msg msg)
{
    struct data       *data = INST_DATA(cl,obj);
    struct DiskObject *icon = data->icon;
    IPTR              res;

    /*
    ** Because of users hate an app that saves on disc
    ** at any exit, we check if you must save something
    */
    if (DoMethod(data->win,MUIM_App_CheckSave))
    {
        DoSuperMethod(cl,obj,MUIM_Application_Save,(IPTR)MUIV_Application_Save_ENV);
        DoSuperMethod(cl,obj,MUIM_Application_Save,(IPTR)MUIV_Application_Save_ENVARC);
    }

    res = DoSuperMethodA(cl,obj,msg);

    if (icon) FreeDiskObject(icon);

    return res;
}

/***********************************************************************/

static Object *findWinObjByAttr(Object *app, ULONG attr, IPTR val)
{
    struct List *winlist;
    Object      *obj;
    Object      *state;

    /* return the window object which supports OM_GET on attr, and
       whose value of attr == val */

    winlist = (struct List *)xget(app,MUIA_Application_WindowList);
    state = (Object *)winlist->lh_Head;

    while((obj = NextObject(&state)) != NULL)
    {
        IPTR value;

        if((value = xget(obj,attr)) != 0 && (value == val))
          break;
    }

    return obj;
}

/**************************************************************************/

static IPTR mOpenWin(UNUSED struct IClass *cl, Object *obj, struct MUIP_App_OpenWin *msg)
{
    Object *win = findWinObjByAttr(obj,msg->IDAttr,msg->IDVal);

    if (!win)
    {
        set(obj,MUIA_Application_Sleep,TRUE);

        win = NewObject(msg->Class,  NULL,
                        msg->IDAttr, msg->IDVal,
                        TAG_MORE,    &msg->InitAttrs);

        if (win) DoMethod(obj,OM_ADDMEMBER,(IPTR)win);

        set(obj,MUIA_Application_Sleep,FALSE);
    }

    return openWindow(obj,win);
}

/**************************************************************************/

static IPTR mCloseWin(struct IClass *cl, Object *obj, struct MUIP_App_CloseWin *msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *win = findWinObjByAttr(obj,msg->IDAttr,msg->IDVal);

    if (win)
    {
        set(win,MUIA_Window_Open,FALSE);
        DoMethod(obj,OM_REMMEMBER,(IPTR)win);
        MUI_DisposeObject(win);

        if (isFlagSet(((struct URL_Node *)msg->IDVal)->Flags, UNF_NEW))
            DoMethod(data->win,MUIM_Win_Delete,msg->IDVal);
    }

    return TRUE;
}

/**************************************************************************/

static IPTR mOpenConfigWindow(struct IClass *cl, Object *obj, Msg msg)
{
    IPTR res;

    set(obj,MUIA_Application_Sleep,TRUE);
    res = DoSuperMethodA(cl,obj,(Msg)msg);
    set(obj,MUIA_Application_Sleep,FALSE);

    return res;
}

/***********************************************************************/

static IPTR mDisposeWin(struct IClass *cl, Object *obj, struct MUIP_App_DisposeWin *msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *win = msg->win;

    set(win,MUIA_Window_Open,FALSE);
    DoSuperMethod(cl,obj,OM_REMMEMBER,(IPTR)win);
    MUI_DisposeObject(win);

    if(win==data->about)
      data->about = NULL;

    return 0;
}

/***********************************************************************/

static IPTR mAbout(struct IClass *cl, Object *obj, UNUSED Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    superset(cl,obj,MUIA_Application_Sleep,TRUE);

    if(data->about == NULL)
    {
        if(g_aboutClass != NULL || initAboutClass() == TRUE)
        {
            if((data->about = aboutObject,
                    MUIA_Aboutmui_Application, obj,
                    MUIA_Window_RefWindow,     data->win,
                End) != NULL)
            {
                DoSuperMethod(cl,obj,OM_ADDMEMBER,(IPTR)data->about);
                DoMethod(data->about,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(IPTR)obj,5,
                    MUIM_Application_PushMethod,(IPTR)obj,2,MUIM_App_DisposeWin,(IPTR)data->about);
            }
        }
    }

    openWindow(obj,data->about);

    superset(cl,obj,MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

static void closeAllWindows(Object *app)
{
  BOOL loop;

  ENTER();

  do
  {
    struct List *list;
    Object *mstate;
    Object *win;

    loop = FALSE;

    list = (struct List *)xget(app, MUIA_Application_WindowList);
    mstate = (Object *)list->lh_Head;

    while((win = NextObject(&mstate)) != NULL)
    {
      ULONG ok = FALSE;

      ok = xget(win, MUIA_App_IsSubWin);
      if(ok)
      {
        set(win, MUIA_Window_Open, FALSE);
        DoMethod(app, OM_REMMEMBER, (IPTR)win);
        MUI_DisposeObject(win);
        loop = TRUE;
        break;
      }
    }
  }
  while(loop == TRUE);

  LEAVE();
}

/***********************************************************************/

static IPTR mGetPrefs(struct IClass *cl, Object *obj, struct MUIP_App_GetPrefs *msg)
{
    struct data *data = INST_DATA(cl,obj);

    set(obj,MUIA_Application_Sleep,TRUE);

    closeAllWindows(obj);
    DoMethod(data->win,MUIM_Win_GetPrefs,msg->mode);

    set(obj,MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

SDISPATCHER(dispatcher)
{
    switch (msg->MethodID)
    {
        case OM_NEW:                            return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:                        return mDispose(cl,obj,(APTR)msg);

        case MUIM_Application_OpenConfigWindow: return mOpenConfigWindow(cl,obj,(APTR)msg);

        case MUIM_App_OpenWin:                  return mOpenWin(cl,obj,(APTR)msg);
        case MUIM_App_CloseWin:                 return mCloseWin(cl,obj,(APTR)msg);
        case MUIM_App_About:                    return mAbout(cl,obj,(APTR)msg);
        case MUIM_App_DisposeWin:               return mDisposeWin(cl,obj,(APTR)msg);
        case MUIM_App_GetPrefs:                 return mGetPrefs(cl,obj,(APTR)msg);

        default:                                return DoSuperMethodA(cl,obj,msg);
    }
}

/**************************************************************************/

BOOL initAppClass(void)
{
    BOOL success = FALSE;

    ENTER();

    if((g_appClass = MUI_CreateCustomClass(NULL, MUIC_Application, NULL, sizeof(struct data), ENTRY(dispatcher))) != NULL)
    {
        localizeNewMenu(menu);
        success = TRUE;
    }

    RETURN(success);
    return success;
}

/**************************************************************************/

void disposeAppClass(void)
{
    if(g_appClass != NULL)
        MUI_DeleteCustomClass(g_appClass);
}

/**************************************************************************/

