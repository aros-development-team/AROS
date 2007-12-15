/*
**  OpenURL - MUI preferences for openurl.library
**
**  Written by Troels Walsted Hansen <troels@thule.no>
**  Placed in the public domain.
**
**  Developed by:
**  - Alfonso Ranieri <alforan@tin.it>
**  - Stefan Kost <ensonic@sonicpulse.de>
**
**  Ported to OS4 by Alexandre Balaban <alexandre@balaban.name>
**
**  Handle the all thing
*/


#include "OpenURL.h"
#define CATCOMP_NUMBERS
#include "loc.h"
#include "OpenURL_rev.h"
#include "libraries/openurl.h"

/**************************************************************************/

struct data
{
    Object            *win;
    Object            *pens;
    Object            *about;
    Object            *aboutMUI;

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
        MITEM(MSG_Menu_LastSaveds),
        MITEM(MSG_Menu_Restore),
        MITEM(MSG_Menu_Defaults),
        MBAR,
        MITEM(MSG_Menu_Pens),
        MBAR,
        MITEM(MSG_Menu_MUI),

    MEND
};

/*
** Used classes
*/

static STRPTR usedClasses[] =
{
    "Textinput.mcc",
    "Urltext.mcc",

    NULL
};

/*
** Here we go
*/
static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object *strip, *win, *pens;

    if (obj = (Object *)DoSuperNew(cl,obj,
            MUIA_Application_Title,       PRG,
            MUIA_Application_Version,     "$VER: "PRGNAME,
            MUIA_Application_Author,      APPAUTHOR,
            MUIA_Application_Copyright,   getString(MSG_App_Copyright),
            MUIA_Application_Description, getString(MSG_App_Description),
            MUIA_Application_HelpFile,    APPHELP,
            MUIA_Application_Base,        APPBASENAME,
            MUIA_Application_Menustrip,   strip = MUI_MakeObject(MUIO_MenustripNM,(ULONG)menu,MUIO_MenustripNM_CommandKeyCheck),
            MUIA_Application_UsedClasses, usedClasses,
            MUIA_Application_Window,      win = winObject, End,
            MUIA_Application_Window,      pens = pensObject, End,
            TAG_MORE,msg->ops_AttrList))
    {
        struct data *data = INST_DATA(cl,obj);

        /*
        ** Setup data
        */

        data->win  = win;
        data->pens = pens;

        if (data->icon = GetDiskObject("PROGDIR:OpenURL"))
            superset(cl,obj,MUIA_Application_DiskObject,data->icon);

        /*
        ** Pens window
        */

        DoMethod(pens,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,MUIV_Notify_Self,3,MUIM_Set,MUIA_Window_Open,FALSE);

        /* Menus */

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_About),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,1,MUIM_App_About);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_AboutMUI),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_Application_AboutMUI,(ULONG)win);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Hide),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,3,MUIM_Set,MUIA_Application_Iconified,TRUE);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Quit),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Save),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)win,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Save);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Use),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)win,2,MUIM_Win_StorePrefs,MUIV_Win_StorePrefs_Use);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_LastSaveds),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_LastSaveds);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Restore),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_Restore);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Defaults),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_App_GetPrefs,MUIV_App_GetPrefs_Defaults);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_MUI),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)obj,2,MUIM_Application_OpenConfigWindow,0);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Pens),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)pens,3,MUIM_Set,MUIA_Window_Open,TRUE);

        /* Menus help */
        DoSuperMethod(cl,obj,MUIM_Notify,MUIA_Application_MenuHelp,MUIV_EveryTime,MUIV_Notify_Self,
            5,MUIM_Application_ShowHelp,(ULONG)win,(ULONG)APPHELP,(ULONG)"MENUS",0);

        /*
        ** Setup default lamps pens
        */
        DoMethod(pens,MUIM_Pens_Change);

        /*
        ** Load list formats and pens prefs
        */
        DoSuperMethod(cl,obj,MUIM_Application_Load,(ULONG)MUIV_Application_Load_ENV);

        /*
        ** Try to get OpenURL prefs and open window
        */
        if (!DoMethod(win,MUIM_Win_GetPrefs,MUIV_Win_GetPrefs_Restore) || !openWindow(obj,win))
        {
            CoerceMethod(cl,obj,OM_DISPOSE);

            return 0;
        }
    }

    return (ULONG)obj;
}

/***********************************************************************/

static ULONG
mDispose(struct IClass *cl,Object *obj,Msg msg)
{
    struct data       *data = INST_DATA(cl,obj);
    struct DiskObject *icon = data->icon;
    ULONG             res;

    /*
    ** Because of users hate an app that saves on disc
    ** at any exit, we check if something changed
    */
    if (DoMethod(data->win,MUIM_App_CheckSave) || DoMethod(data->pens,MUIM_App_CheckSave))
    {
        DoSuperMethod(cl,obj,MUIM_Application_Save,(ULONG)MUIV_Application_Save_ENV);
        DoSuperMethod(cl,obj,MUIM_Application_Save,(ULONG)MUIV_Application_Save_ENVARC);
    }

    res = DoSuperMethodA(cl,obj,msg);

    if (icon) FreeDiskObject(icon);

    return res;
}

/***********************************************************************/
/*
** We just care of pens
*/

static ULONG
mSets(struct IClass *cl,Object *obj,struct opSet *msg)
{
    struct MUI_PenSpec **specs;

    if (specs = (struct MUI_PenSpec **)GetTagData(MUIA_App_Pens,(ULONG)NULL,msg->ops_AttrList))
    {
        struct data *data = INST_DATA(cl,obj);

        set(data->win,MUIA_App_Pens,specs);
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static Object *
findWinObjByAttr(Object *app,ULONG attr,ULONG val)
{
    struct List *winlist;
    Object      *obj;
    APTR        state;

    /* return the window object which supports OM_GET on attr, and
       whose value of attr == val */

    get(app,MUIA_Application_WindowList,&winlist);
    state = winlist->lh_Head;

    while(obj = NextObject(&state))
    {
        ULONG value;

        if (get(obj,attr,&value) && (value==val)) break;
    }

    return obj;
}

/**************************************************************************/

static ULONG
mOpenWin(struct IClass *cl,Object *obj,struct MUIP_App_OpenWin *msg)
{
    Object *win = findWinObjByAttr(obj,msg->IDAttr,msg->IDVal);

    if (!win)
    {
        set(obj,MUIA_Application_Sleep,TRUE);

        win = NewObject(msg->Class,  NULL,
                        msg->IDAttr, msg->IDVal,
                        TAG_MORE,    &msg->InitAttrs);

        if (win) DoMethod(obj,OM_ADDMEMBER,(ULONG)win);

        set(obj,MUIA_Application_Sleep,FALSE);
    }

    return openWindow(obj,win);
}

/**************************************************************************/

static ULONG
mCloseWin(struct IClass *cl,Object *obj,struct MUIP_App_CloseWin *msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *win = findWinObjByAttr(obj,msg->IDAttr,msg->IDVal);

    if (win)
    {
        set(win,MUIA_Window_Open,FALSE);
        DoMethod(obj,OM_REMMEMBER,(ULONG)win);
        MUI_DisposeObject(win);

        if (((struct URL_Node *)msg->IDVal)->Flags & UNF_NEW)
            DoMethod(data->win,MUIM_Win_Delete,msg->IDVal);
    }

    return TRUE;
}

/**************************************************************************/

static ULONG
mOpenConfigWindow(struct IClass *cl,Object *obj,Msg msg)
{
    ULONG res;

    set(obj,MUIA_Application_Sleep,TRUE);
    res = DoSuperMethodA(cl,obj,(Msg)msg);
    set(obj,MUIA_Application_Sleep,FALSE);

    return res;
}

/***********************************************************************/

static ULONG
mDisposeWin(struct IClass *cl,Object *obj,struct MUIP_App_DisposeWin *msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *win = msg->win;

    set(win,MUIA_Window_Open,FALSE);
    DoSuperMethod(cl,obj,OM_REMMEMBER,(ULONG)win);
    MUI_DisposeObject(win);

    if (win==data->about) data->about = NULL;
    else if (win==data->aboutMUI) data->aboutMUI = NULL;

    return 0;
}

/***********************************************************************/

static ULONG
mAboutMUI(struct IClass *cl,Object *obj, Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    superset(cl,obj,MUIA_Application_Sleep,TRUE);

    if (!data->aboutMUI)
    {
        if (data->aboutMUI = AboutmuiObject,
                MUIA_HelpNode,             "MUI",
                MUIA_Aboutmui_Application, obj,
                MUIA_Window_RefWindow,     data->win,
            End)
            DoMethod(data->aboutMUI,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(ULONG)obj,5,
                MUIM_Application_PushMethod,(ULONG)obj,2,MUIM_App_DisposeWin,(ULONG)data->aboutMUI);
    }

    openWindow(obj,data->aboutMUI);

    superset(cl,obj,MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

static ULONG
mAbout(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    superset(cl,obj,MUIA_Application_Sleep,TRUE);

    if (!data->about)
    {
        if (g_aboutClass || initAboutClass())
        {
            if (data->about = aboutObject,
                    MUIA_Aboutmui_Application, obj,
                    MUIA_Window_RefWindow,     data->win,
                End)
            {
                DoSuperMethod(cl,obj,OM_ADDMEMBER,(ULONG)data->about);
                DoMethod(data->about,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(ULONG)obj,5,
                    MUIM_Application_PushMethod,(ULONG)obj,2,MUIM_App_DisposeWin,(ULONG)data->about);
    	    }
        }
    }

    openWindow(obj,data->about);

    superset(cl,obj,MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

static void
closeAllWindows(Object *app)
{
    ULONG loop;

    do
    {
        struct List *list;
        Object      *mstate;
        Object      *win;

        loop = FALSE;

        get(app,MUIA_Application_WindowList,&list);
        mstate = (Object *)list->lh_Head;

        while (win = NextObject(&mstate))
        {
            ULONG ok = FALSE;

            get(win,MUIA_App_IsSubWin,&ok);
            if (ok)
            {
                set(win,MUIA_Window_Open,FALSE);
                DoMethod(app,OM_REMMEMBER,(ULONG)win);
                MUI_DisposeObject(win);
                loop = TRUE;
                break;
            }
        }

    }
    while (loop);
}

/***********************************************************************/

static ULONG
mGetPrefs(struct IClass *cl,Object *obj,struct MUIP_App_GetPrefs *msg)
{
    struct data *data = INST_DATA(cl,obj);

    set(obj,MUIA_Application_Sleep,TRUE);

    closeAllWindows(obj);
    DoMethod(data->win,MUIM_Win_GetPrefs,msg->mode);

    set(obj,MUIA_Application_Sleep,FALSE);

    return 0;
}

/***********************************************************************/

M_DISP(dispatcher)
{
    M_DISPSTART

    switch (msg->MethodID)
    {
        case OM_NEW:                            return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:                        return mDispose(cl,obj,(APTR)msg);
        case OM_SET:                            return mSets(cl,obj,(APTR)msg);

        case MUIM_Application_AboutMUI:         return mAboutMUI(cl,obj,(APTR)msg);
        case MUIM_Application_OpenConfigWindow: return mOpenConfigWindow(cl,obj,(APTR)msg);

        case MUIM_App_OpenWin:                  return mOpenWin(cl,obj,(APTR)msg);
        case MUIM_App_CloseWin:                 return mCloseWin(cl,obj,(APTR)msg);
        case MUIM_App_About:                    return mAbout(cl,obj,(APTR)msg);
        case MUIM_App_DisposeWin:               return mDisposeWin(cl,obj,(APTR)msg);
        case MUIM_App_GetPrefs:                 return mGetPrefs(cl,obj,(APTR)msg);

        default:                                return DoSuperMethodA(cl,obj,msg);
    }
}

M_DISPEND(dispatcher)

/**************************************************************************/

ULONG
initAppClass(void)
{
    if (g_appClass = MUI_CreateCustomClass(NULL,MUIC_Application,NULL,sizeof(struct data),DISP(dispatcher)))
    {
        localizeNewMenu(menu);

        return TRUE;
    }

    return FALSE;
}

/**************************************************************************/

void
disposeAppClass(void)
{
    if (g_appClass) MUI_DeleteCustomClass(g_appClass);
}

/**************************************************************************/

