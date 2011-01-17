
#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/icon.h>

#include <libraries/mui.h>
#include <libraries/gadtools.h>

#include "muimiamipanel_intern.h"
#include "muimiamipanel_locale.h"
#include "muimiamipanel_misc.h"
#include "muimiamipanel_iffprefs.h"
#include "muimiamipanel_commands.h"

#include <mui/BWin_mcc.h>
#include <mui/TheBar_mcc.h>

#include "Classes/muimiamipanel_classes.h"

/***********************************************************************/

struct MiamiPanelAppClass_DATA
{
    Object            *win;
    Object            *root;
    Object            *about;
    Object            *wprefs;
    Object            *aboutMUI;

    struct DiskObject *icon;

    struct MPS_Prefs  prefs;
};

/***********************************************************************/

static ULONG AppMenuIDs[] =
{
    MSG_Menu_Project,
       MSG_Menu_About,
        MSG_Menu_AboutMUI,
        0,
        MSG_Menu_Hide,
        0,
        MSG_Menu_Quit,

    MSG_Menu_Miami,
        MSG_Menu_Miami_Show,
        MSG_Menu_Miami_Hide,
        0,
        MSG_Menu_Miami_Quit,

    MSG_Menu_Prefs,
        MSG_Menu_Edit,
        MSG_Menu_Save,
        MSG_Menu_Use,
        MSG_Menu_Restore,
        MSG_Menu_LastSaved,
        0,
        MSG_Menu_MUI
};

static struct NewMenu appMenu[] =
{
    MTITLE(MSG_Menu_Project),
        MITEM(MSG_Menu_About),
        MITEM(MSG_Menu_AboutMUI),
        MBAR,
        MITEM(MSG_Menu_Hide),
        MBAR,
        MITEM(MSG_Menu_Quit),

    MTITLE(MSG_Menu_Miami),
        MITEM(MSG_Menu_Miami_Show),
        MITEM(MSG_Menu_Miami_Hide),
        MBAR,
        MITEM(MSG_Menu_Miami_Quit),

    MTITLE(MSG_Menu_Prefs),
        MITEM(MSG_Menu_Edit),
        MITEM(MSG_Menu_Save),
        MITEM(MSG_Menu_Use),
        MITEM(MSG_Menu_Restore),
        MITEM(MSG_Menu_LastSaved),
        MBAR,
        MITEM(MSG_Menu_MUI),

    MEND
};

static UBYTE *usedClasses[] =
{
    "Busy.mcc",
    "Bwin.mcc",
    "Lamp.mcc",
    "TheBar.mcc",
    "Urltext.mcc",
    NULL
};

static struct MiamiPanelBase_intern *MiamiPanelBaseIntern;

IPTR MUIPC_App__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct MiamiPanelAppClass_DATA    temp;
    struct TagItem *attrs = message->ops_AttrList;
    Object         *strip;

    memset(&temp,0,sizeof(temp));

    if (!loadIFFPrefs(MPV_LoadPrefs_Env|MPV_LoadPrefs_FallBack, &temp.prefs, MiamiPanelBaseIntern))
    {
        temp.prefs.layout    = DEF_Layout;
        temp.prefs.flags     = DEF_Flags;
        temp.prefs.barLayout = DEF_TBLayout;
        temp.prefs.viewMode  = DEF_TBVMode;
        temp.prefs.labelPos  = DEF_TBLPos;
        temp.prefs.btflags   = DEF_TBFlags;
    }

    if (self = (Object *)DoSuperNewTags
		(
			CLASS, self, NULL,

            MUIA_Application_Title,          DEF_Base,
            MUIA_Application_Author,         DEF_Author,
            /* MUIA_Application_Version,        lib_vers, */
            MUIA_Application_Copyright,      __(MSG_Copyright),
            MUIA_Application_Description,    __(MSG_Description),
            MUIA_Application_Base,           DEF_Base,
            MUIA_Application_HelpFile,       DEF_Guide,
            MUIA_Application_UseCommodities, FALSE,
            MUIA_Application_SingleTask,     TRUE,
            MUIA_Application_Menustrip,      strip = MUI_MakeObject(MUIO_MenustripNM, (ULONG)appMenu, MUIO_MenustripNM_CommandKeyCheck),
            MUIA_Application_UsedClasses,    usedClasses,

            SubWindow, temp.win = (temp.prefs.flags & MPV_Flags_BWin) ?
                (BWinObject,
                    MUIA_ObjectID,     MAKE_ID('M','A','I','N'),
                    MUIA_Window_ID,    MAKE_ID('M','A','I','N'),
                    MUIA_Window_Title, __(MSG_Window_Title),
                    MUIA_HelpNode,     "GUI",
                    MUIA_BWin_Save,    MUIV_BWin_Save_All,
                    MUIA_BWin_Borders, temp.prefs.flags & MPV_Flags_BWinBorders,
                    WindowContents, temp.root = VGroup,
                        InnerSpacing(0,0),
                        Child, (IPTR)NewObject(MiamiPanelBaseIntern->mpb_mgroupClass->mcc_Class, NULL ,
                            MPA_Prefs, &temp.prefs,
                            MPA_Show,  GetTagData(MPA_Show, 0, message->ops_AttrList),
                        TAG_DONE),
                    End,
                 End) :
                (WindowObject,
                    MUIA_ObjectID,                      MAKE_ID('M','A','I','N'),
                    MUIA_Window_ID,                     MAKE_ID('M','A','I','N'),
                    MUIA_HelpNode,                      "GUI",
                    MUIA_Window_Title,                  __(MSG_Window_Title),
                    MUIA_Window_UseRightBorderScroller, TRUE,
                    MUIA_Window_SizeRight,              TRUE,
                    //MUIA_Window_Backdrop,               TRUE,

                    WindowContents, temp.root = VGroup,
                        InnerSpacing(0,0),
                        Child, (IPTR)NewObject(MiamiPanelBaseIntern->mpb_mgroupClass->mcc_Class, NULL,
                            MPA_Prefs, &temp.prefs,
                            MPA_Show,  GetTagData(MPA_Show, 0, message->ops_AttrList),
                        TAG_DONE),
                    End,
                 End),
            TAG_MORE,attrs))
    {
        struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

        /* Setup istance */
        CopyMem(&temp,data,sizeof(struct MiamiPanelAppClass_DATA));

        NEWLIST(&data->prefs.iflist);
        moveMinList(&data->prefs.iflist, &temp.prefs.iflist, MiamiPanelBaseIntern);

        if (data->icon = GetDiskObject(DEF_Icon))
			SetSuperAttrs(CLASS, self, MUIA_Application_DiskObject,data->icon,TAG_DONE);
        /* Load MUI preferences */
        DoSuperMethod(CLASS,self,MUIM_Application_Load,(ULONG)MUIV_Application_Load_ENV);

        /* Win notifies */
        DoMethod(temp.win,MUIM_Notify,MUIA_Window_CloseRequest,MUIV_EveryTime,(ULONG)self,1,MPM_Quit);

        /* Menus notifies */
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_About),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,1,MPM_About);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_AboutMUI),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MUIM_Application_AboutMUI,(ULONG)temp.win);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Hide),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,3,MUIM_Set,MUIA_Application_Iconified,TRUE);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Quit),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,1,MPM_Quit);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Edit),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,1,MPM_Prefs);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_MUI),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MUIM_Application_OpenConfigWindow,0);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Save),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Save,TRUE);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Use),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Save,FALSE);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Restore),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Load,FALSE);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_LastSaved),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Load,TRUE);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Miami_Show),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Miami,MPV_Miami_Show);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Miami_Hide),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Miami,MPV_Miami_Hide);

        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MSG_Menu_Miami_Quit),MUIM_Notify,
            MUIA_Menuitem_Trigger,MUIV_EveryTime,(ULONG)self,2,MPM_Miami,MPV_Miami_Quit);

        DoSuperMethod(CLASS,self,MUIM_Notify,MUIA_Application_MenuHelp,MUIV_EveryTime,MUIV_Notify_Self,
            5,MUIM_Application_ShowHelp,(ULONG)temp.win,(ULONG)DEF_Guide,(ULONG)"Menus",0);

        /* Set root preferences */
        set(data->root,MPA_Prefs,&temp.prefs);

        /* Try to open win */
        if (!openWindow(self, temp.win, MiamiPanelBaseIntern))
        {
            CoerceMethod(CLASS, self, OM_DISPOSE);
            self = NULL;
        }
    }

    return (ULONG)self;
}

/***********************************************************************/

ULONG MUIPC_App__OM_DISPOSE
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

    freeIFList(&data->prefs, MiamiPanelBaseIntern);
    if (data->icon) FreeDiskObject(data->icon);

    return DoSuperMethodA(CLASS,self,message);
}

/***********************************************************************/

static ULONG MUIPC_App__OM_GET
(
    Class *CLASS, Object *self, struct opGet *message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

    switch (message->opg_AttrID)
    {
        case MPA_Prefs: *message->opg_Storage = (ULONG)&data->prefs; return TRUE;
        default: return DoSuperMethodA(CLASS,self,(Msg)message);
    }
}

/***********************************************************************/

static ULONG MUIPC_App__OM_SET
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct MiamiPanelAppClass_DATA      *data = INST_DATA(CLASS,self);
    struct MPS_Prefs *prefs = NULL;
    struct TagItem   *tag, *ticonify = NULL; // gcc
    struct TagItem   *tstate;
    ULONG            close = FALSE, oneWay = FALSE, noIfList = FALSE;

    for (tstate = message->ops_AttrList; tag = NextTagItem(&tstate); )
    {
        ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MPA_OneWay:
                oneWay = TRUE;
                break;

            case MPA_NoIfList:
                noIfList = TRUE;
                break;

            case MPA_Prefs:
                prefs = PREFS(tidata);
                break;

            case MUIA_Application_Iconified:
                ticonify = tag;
                close = tidata;
                break;

            case MPA_Bar:
                set(data->root,MPA_Bar,tidata);
                break;
        }
    }

    if (prefs)
    {
        struct List *l;
        Object      *win;
        Object      *mstate;
        ULONG       bwin, bwinBorders;

        bwin = prefs->flags & MPV_Flags_BWin;
        bwinBorders = prefs->flags & MPV_Flags_BWinBorders;

        if (!BOOLSAME(bwin,data->prefs.flags & MPV_Flags_BWin) || !BOOLSAME(bwinBorders,data->prefs.flags & MPV_Flags_BWinBorders))
        {
            set(data->win,MUIA_Window_Open,FALSE);
            DoSuperMethod(CLASS,self,MUIM_Application_PushMethod,(ULONG)self,1,MPM_Rebuild);
        }

        freeIFList(&data->prefs, MiamiPanelBaseIntern);
        CopyMem(prefs,&data->prefs,sizeof(data->prefs));
        NEWLIST(&data->prefs.iflist);
        if (noIfList) DoMethod(data->root,MPM_MGroup_GrabIFList);
        else moveMinList(&data->prefs.iflist,&prefs->iflist, MiamiPanelBaseIntern);

        for (get(self,MUIA_Application_WindowList,&l), mstate = (Object *)l->lh_Head; win = NextObject(&mstate); )
            if (!oneWay || (win!=data->wprefs)) set(win,MPA_Prefs,&data->prefs);

        set(data->root,MPA_Prefs,&data->prefs);
    }

    if (close && !(data->prefs.flags & MPV_Flags_Iconify))
    {
        ticonify->ti_Tag = TAG_IGNORE;
        DoSuperMethod(CLASS,self,MUIM_Application_PushMethod,(ULONG)self,1,MPM_Quit);
    }

    return DoSuperMethodA(CLASS,self,(Msg)message);
}

/***********************************************************************/

static ULONG MUIPC_App__MUIM_Application_AboutMUI
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, TRUE, TAG_DONE);

    if (!data->aboutMUI)
    {
        if (data->aboutMUI = AboutmuiObject,
                MUIA_Aboutmui_Application, self,
                MUIA_Window_RefWindow,     data->win,
                MUIA_HelpNode,             "MUI",
            End)
            DoMethod(data->aboutMUI,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(ULONG)self,5,
                MUIM_Application_PushMethod,(ULONG)self,2,MPM_DisposeWin,(ULONG)data->aboutMUI);
    }

    openWindow(self, data->aboutMUI, MiamiPanelBaseIntern);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, FALSE, TAG_DONE);
	
    return 0;
}

/***********************************************************************/

static ULONG MUIPC_App__MUIM_Application_OpenConfigWindow
(
    Class *CLASS, Object *self, Msg message
)
{
    ULONG res;

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, TRUE, TAG_DONE);
    res = DoSuperMethodA(CLASS,self,message);
	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, FALSE, TAG_DONE);
	
    return res;
}

/***********************************************************************/

static ULONG MUIPC_App__MPM_Rebuild
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);
    Object      *owin = data->win, *nwin, *sp = NULL; // gcc
    ULONG       bwin = data->prefs.flags & MPV_Flags_BWin;
    ULONG       res;

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, TRUE, TAG_DONE);

    set(owin,MUIA_Window_Open,FALSE);
    set(owin,WindowContents,NULL);

    if (bwin)
        nwin = BWinObject,
            MUIA_ObjectID,     MAKE_ID('M','A','I','N'),
            MUIA_Window_ID,    MAKE_ID('M','A','I','N'),
            MUIA_Window_Title, __(MSG_Window_Title),
            MUIA_HelpNode,     "GUI",
            MUIA_BWin_Save,    MUIV_BWin_Save_All,
            MUIA_BWin_Borders, data->prefs.flags & MPV_Flags_BWinBorders,
        End;
    else
        nwin = WindowObject,
            MUIA_ObjectID,                      MAKE_ID('M','A','I','N'),
            MUIA_Window_ID,                     MAKE_ID('M','A','I','N'),
            MUIA_HelpNode,                      "GUI",
            MUIA_Window_Title,                  __(MSG_Window_Title),
            MUIA_Window_UseRightBorderScroller, TRUE,
            MUIA_Window_SizeRight,              TRUE,
            WindowContents,                     sp = HSpace(0),
        End;

    if (nwin)
    {
        DoSuperMethod(CLASS,self,OM_REMMEMBER,(ULONG)owin);
        MUI_DisposeObject(owin);
        DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)(data->win = nwin));
        DoMethod(nwin,MUIM_Notify,MUIA_Window_CloseRequest,MUIV_EveryTime,(ULONG)self,1,MPM_Quit);
        res = TRUE;
        if (!bwin)
        {
        set(nwin,WindowContents,NULL);
            MUI_DisposeObject(sp);
        }
    }
    else
    {
        if (bwin) data->prefs.flags &= ~MPV_Flags_BWin;
        else data->prefs.flags |= MPV_Flags_BWin;
        if (data->wprefs) set(data->wprefs,MPA_Prefs,&data->prefs);
        res = FALSE;

    }

    set(data->win,WindowContents,data->root);

    if (!openWindow(self, data->win, MiamiPanelBaseIntern)) DoSuperMethod(CLASS,self,MUIM_Application_PushMethod,(ULONG)self,1,MPM_Quit);
	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, FALSE, TAG_DONE);

    return res;
}

/***********************************************************************/

static ULONG MUIPC_App__MPM_Quit
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

    set(data->win,MUIA_Window_Open,FALSE);
    if (data->about) set(data->about,MUIA_Window_Open,FALSE);
    if (data->wprefs) set(data->wprefs,MUIA_Window_Open,FALSE);

    MiamiPanelFun(MiamiPanelBaseIntern, MIAMIPANELV_CallBack_Code_ClosePanel);

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_App__MPM_DisposeWin(struct IClass *CLASS,Object *self,struct MPP_DisposeWin *message)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);
    Object      *win = message->win;

    set(win,MUIA_Window_Open,FALSE);
    DoSuperMethod(CLASS,self,OM_REMMEMBER,(ULONG)win);
    MUI_DisposeObject(win);

    if (win==data->about) data->about = NULL;
    else if (win==data->wprefs) data->wprefs = NULL;
         else if (win==data->aboutMUI) data->aboutMUI = NULL;

    return 0;
}

/***********************************************************************/

static ULONG MUIPC_App__MPM_About
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, TRUE, TAG_DONE);

    if (!data->about)
    {
        ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

        if (MiamiPanelBaseIntern->mpb_aboutClass || MUIPC_About_ClassInit(MiamiPanelBaseIntern))
        {
            if (data->about = NewObject(MiamiPanelBaseIntern->mpb_aboutClass->mcc_Class,
														NULL,
														MPA_Application,       self,
														MUIA_Window_RefWindow, data->win,
														TAG_DONE))
            {
                DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)data->about);

                DoMethod(data->about,MUIM_Notify,MUIA_Window_CloseRequest,TRUE,(ULONG)self,5,
                    MUIM_Application_PushMethod,(ULONG)self,2,MPM_DisposeWin,(ULONG)data->about);
            }
        }

        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
    }

    openWindow(self, data->about, MiamiPanelBaseIntern);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, FALSE, TAG_DONE);

    return 0;
}

/***********************************************************************/

static ULONG MUIPC_App__MPM_Prefs
(
    Class *CLASS, Object *self, Msg message
)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, TRUE, TAG_DONE);
	
    if (!data->wprefs)
    {
        ObtainSemaphore(&MiamiPanelBaseIntern->mpb_libSem);

        if (MiamiPanelBaseIntern->mpb_prefsClass || MUIPC_Prefs_ClassInit(MiamiPanelBaseIntern))
        {
            if (data->wprefs = NewObject(MiamiPanelBaseIntern->mpb_prefsClass->mcc_Class,
															NULL,
															MPA_Prefs, &data->prefs,
															TAG_DONE))
			DoSuperMethod(CLASS,self,OM_ADDMEMBER,(ULONG)data->wprefs);
        }

        ReleaseSemaphore(&MiamiPanelBaseIntern->mpb_libSem);
    }

    openWindow(self, data->wprefs, MiamiPanelBaseIntern);

	SetSuperAttrs(CLASS, self, MUIA_Application_Sleep, FALSE, TAG_DONE);

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_App__MPM_Save(struct IClass *CLASS,Object *self,struct MPP_Save *message)
{
    struct MiamiPanelAppClass_DATA *data = INST_DATA(CLASS,self);

    DoMethod(data->root,MPM_MGroup_GrabIFList);

    if (message->save)
    {
        saveIFFPrefs(DEF_ENVARCFILE, &data->prefs, MiamiPanelBaseIntern);
        DoSuperMethod(CLASS,self,MUIM_Application_Save,(ULONG)MUIV_Application_Save_ENVARC);
    }

    saveIFFPrefs(DEF_ENVFILE, &data->prefs, MiamiPanelBaseIntern);
    DoSuperMethod(CLASS,self,MUIM_Application_Save,(ULONG)MUIV_Application_Save_ENV);

    DoMethod(data->win,MUIM_Window_Snapshot,1);

    return 0;
}

/***********************************************************************/

static ULONG
MUIPC_App__MPM_Load(struct IClass *CLASS, Object *self,struct MPP_Load *message)
{
    struct MPS_Prefs prefs;
    ULONG            pres, res;

    if (message->envarc) pres = loadIFFPrefs(MPV_LoadPrefs_EnvArc, &prefs, MiamiPanelBaseIntern);
    else pres = loadIFFPrefs(MPV_LoadPrefs_Env, &prefs, MiamiPanelBaseIntern);

    res = DoSuperMethod(CLASS,self,MUIM_Application_Load,(ULONG)(message->envarc ? MUIV_Application_Load_ENVARC : MUIV_Application_Load_ENV));

    if (pres) set(self,MPA_Prefs,&prefs);
    else DisplayBeep(0);

    return res;
}

/***********************************************************************/

static ULONG
MUIPC_App__MPM_Miami(struct IClass *CLASS,Object *self,struct MPP_Miami *message)
{
    ULONG cmd = 0; // gcc

    switch(message->cmd)
    {
        case MPV_Miami_Show: cmd = MIAMIPANELV_CallBack_Code_ShowMainGUI; break;
        case MPV_Miami_Hide: cmd = MIAMIPANELV_CallBack_Code_HideMainGUI; break;
        case MPV_Miami_Quit: cmd = MIAMIPANELV_CallBack_Code_QuitMiami; break;
    }

    MiamiPanelFun(MiamiPanelBaseIntern, cmd);

    return 0;
}

/***********************************************************************/

BOOPSI_DISPATCHER(IPTR, MUIPC_App_Dispatcher, CLASS, self, message)
{
    switch(message->MethodID)
    {
        case OM_NEW:                                                     return MUIPC_App__OM_NEW(CLASS,self,(APTR)message);
        case OM_GET:                                                      return MUIPC_App__OM_GET(CLASS,self,(APTR)message);
        case OM_SET:                                                       return MUIPC_App__OM_SET(CLASS,self,(APTR)message);
        case OM_DISPOSE:                                               return MUIPC_App__OM_DISPOSE(CLASS,self,(APTR)message);

        case MUIM_Application_AboutMUI:                        return MUIPC_App__MUIM_Application_AboutMUI(CLASS,self,(APTR)message);
        case MUIM_Application_OpenConfigWindow:       return MUIPC_App__MUIM_Application_OpenConfigWindow(CLASS,self,(APTR)message);

        case MPM_Rebuild:                                                return MUIPC_App__MPM_Rebuild(CLASS,self,(APTR)message);
        case MPM_Quit:                                                     return MUIPC_App__MPM_Quit(CLASS,self,(APTR)message);
        case MPM_DisposeWin:                                         return MUIPC_App__MPM_DisposeWin(CLASS,self,(APTR)message);
        case MPM_About:                                                   return MUIPC_App__MPM_About(CLASS,self,(APTR)message);
        case MPM_Prefs:                                                    return MUIPC_App__MPM_Prefs(CLASS,self,(APTR)message);
        case MPM_Save:                                                    return MUIPC_App__MPM_Save(CLASS,self,(APTR)message);
        case MPM_Load:                                                    return MUIPC_App__MPM_Load(CLASS,self,(APTR)message);
        case MPM_Miami:                                                  return MUIPC_App__MPM_Miami(CLASS,self,(APTR)message);

        default:                                                                return DoSuperMethodA(CLASS,self,message);
    }
	return 0;
}
BOOPSI_DISPATCHER_END

/***********************************************************************/

ULONG
MUIPC_App_ClassInit(struct MiamiPanelBase_intern *MiamiPanelBase)
{
	MiamiPanelBaseIntern = MiamiPanelBase;
    if ((MiamiPanelBaseIntern->mpb_appClass = MUI_CreateCustomClass(NULL, MUIC_Application, NULL, sizeof(struct MiamiPanelAppClass_DATA), MUIPC_App_Dispatcher)))
    {
        localizeMenus(appMenu,AppMenuIDs);

        return TRUE;
    }

    return FALSE;
}

/***********************************************************************/
