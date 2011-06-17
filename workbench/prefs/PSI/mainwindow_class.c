/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/dos.h>
#include <proto/alib.h>
#include <proto/asl.h>
#include <proto/muimaster.h>
#include <proto/muiscreen.h>
#include <proto/intuition.h>

#include <string.h>

#include "screenpanel_class.h"
#include "screenlist_class.h"
#include "mainwindow_class.h"

/****************************************************************************************/

enum { MEN_OPEN=1, MEN_APPEND, MEN_SAVEAS, MEN_ABOUT, MEN_QUIT, MEN_LASTSAVED, MEN_RESTORE,MEN_MUI};

struct NewMenu MainMenu[] =
{
    { NM_TITLE, (STRPTR)MSG_MENU_PROJECT          , 0 ,0,0,(APTR)0             },
    { NM_ITEM , (STRPTR)MSG_MENU_PROJECT_OPEN     ,"O",0,0,(APTR)MEN_OPEN      },
    { NM_ITEM , (STRPTR)MSG_MENU_PROJECT_APPEND   ,"P",0,0,(APTR)MEN_APPEND    },
    { NM_ITEM , (STRPTR)MSG_MENU_PROJECT_SAVEAS   ,"A",0,0,(APTR)MEN_SAVEAS    },
    { NM_ITEM , (STRPTR)NM_BARLABEL               , 0 ,0,0,(APTR)0             },
    { NM_ITEM , (STRPTR)MSG_MENU_PROJECT_ABOUT    ,"?",0,0,(APTR)MEN_ABOUT     },
    { NM_ITEM , (STRPTR)NM_BARLABEL               , 0 ,0,0,(APTR)0             },
    { NM_ITEM , (STRPTR)MSG_MENU_PROJECT_QUIT     ,"Q",0,0,(APTR)MEN_QUIT      },

    { NM_TITLE, (STRPTR)MSG_MENU_EDIT             , 0 ,0,0,(APTR)0             },
    { NM_ITEM , (STRPTR)MSG_MENU_EDIT_LASTSAVED   ,"L",0,0,(APTR)MEN_LASTSAVED },
    { NM_ITEM , (STRPTR)MSG_MENU_EDIT_RESTORE     ,"R",0,0,(APTR)MEN_RESTORE   },

    { NM_TITLE, (STRPTR)MSG_MENU_SETTINGS         , 0 ,0,0,(APTR)0             },
    { NM_ITEM , (STRPTR)MSG_MENU_SETTINGS_MUI     , 0 ,0,0,(APTR)MEN_MUI       },

    { NM_END,NULL,0,0,0,(APTR)0 },
};

/****************************************************************************************/

struct MainWindow_Data
{
    Object *PA_Screens;
};

/****************************************************************************************/

IPTR MainWindow_Finish(struct IClass *cl, Object *obj, struct MUIP_MainWindow_Finish *msg)
{
    struct MainWindow_Data *data = INST_DATA(cl, obj);
    if (msg->level >= 1)
        DoMethod(data->PA_Screens, MUIM_ScreenList_Save, PSD_FILENAME_USE );
    if (msg->level>=2)
        DoMethod(data->PA_Screens, MUIM_ScreenList_Save, PSD_FILENAME_SAVE);
    DoMethod
        ((Object *)xget(obj, MUIA_ApplicationObject),
        MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit
    );

    return 0;
}

/****************************************************************************************/

IPTR MainWindow_About(struct IClass *cl, Object *obj, Msg msg)
{
    struct MainWindow_Data *data = INST_DATA(cl, obj);
    Object *app = (Object *)xget(obj, MUIA_ApplicationObject);
    static const char AboutText[] = "\33b\33cPSI - Public Screen Inspector\33n\n\33cVersion: %s\n\33c%s\n\n\33c%s %ld\n\33cARexx-Port: %s";
    MUI_Request
    (
        app, obj, 0, NULL, GetStr(MSG_BUTTON_OK), (char *)AboutText,
        ((char *)xget(app, MUIA_Application_Version)) + 10,
        ((char *)xget(app, MUIA_Application_Copyright)),
        GetStr(MSG_ABOUT_NUMBEROFSCREENS),
        xget(data->PA_Screens, MUIA_List_Entries),
        ((char *)xget(app, MUIA_Application_Base))
    );

    return 0;
}

/****************************************************************************************/

VOID IntuiMsgFunc(struct Hook *hook, struct FileRequester *req, struct IntuiMessage *imsg)
{
    if (imsg->Class == IDCMP_REFRESHWINDOW)
        DoMethod(req->fr_UserData, MUIM_Application_CheckRefresh);
}

/****************************************************************************************/

CONST_STRPTR getfilename(Object *win, CONST_STRPTR title, BOOL save)
{
    static char buf[512];
    struct FileRequester *req;
    struct Window *w = NULL;
    static LONG left = -1, top = -1, width = -1, height = -1;
    Object *app = (Object *)xget(win, MUIA_ApplicationObject);
    char *res = NULL;
    static struct Hook IntuiMsgHook;

    IntuiMsgHook.h_Entry = HookEntry;
    IntuiMsgHook.h_SubEntry = (HOOKFUNC)IntuiMsgFunc;

    get(win, MUIA_Window_Window, &w);
    if (left == -1)
    {
        left   = w->LeftEdge + w->BorderLeft + 2;
        top    = w->TopEdge + w->BorderTop + 2;
        width  = w->Width - w->BorderLeft - w->BorderRight - 4;
        height = w->Height - w->BorderTop - w->BorderBottom - 4;
    }

    if ((req = MUI_AllocAslRequestTags(ASL_FileRequest,
        ASLFR_Window, w,
        ASLFR_TitleText, title,
        ASLFR_InitialLeftEdge, left,
        ASLFR_InitialTopEdge , top,
        ASLFR_InitialWidth   , width,
        ASLFR_InitialHeight  , height,
        ASLFR_InitialDrawer  , "envarc:Zune",
        ASLFR_InitialPattern , "#?.iff",
        ASLFR_DoSaveMode     , save,
        ASLFR_DoPatterns     , TRUE,
        ASLFR_RejectIcons    , TRUE,
        ASLFR_UserData       , app,
        ASLFR_IntuiMsgFunc   , &IntuiMsgHook,
        TAG_DONE)))
    {
        set(app, MUIA_Application_Sleep, TRUE);
        if (MUI_AslRequestTags(req, TAG_DONE))
        {
            if (*req->fr_File)
            {
                res = buf;
                stccpy(buf, req->fr_Drawer, sizeof(buf));
                AddPart(buf, req->fr_File, sizeof(buf));
            }
            left   = req->fr_LeftEdge;
            top    = req->fr_TopEdge;
            width  = req->fr_Width;
            height = req->fr_Height;
        }
        MUI_FreeAslRequest(req);
        set(app, MUIA_Application_Sleep, FALSE);
    }

    return res;
}

/****************************************************************************************/

IPTR MainWindow_Open(struct IClass *cl, Object *obj, struct MUIP_MainWindow_Open *msg)
{
    struct MainWindow_Data *data = INST_DATA(cl, obj);
    CONST_STRPTR title = msg->append ? GetStr(MSG_TITLE_APPEND) : GetStr(MSG_TITLE_OPEN);
    CONST_STRPTR name;
    if ((name = getfilename(obj, title, FALSE)) && *name)
    {
        if (!msg->append) DoMethod(data->PA_Screens, MUIM_ScreenPanel_CloseWindows);
        DoMethod(data->PA_Screens, MUIM_ScreenList_Load, name, msg->append ? FALSE : TRUE);
    }

    return 0;
}

/****************************************************************************************/

IPTR MainWindow_SaveAs(struct IClass *cl, Object *obj, Msg msg)
{
    struct MainWindow_Data *data = INST_DATA(cl, obj);
    CONST_STRPTR title = GetStr(MSG_TITLE_SAVE);
    CONST_STRPTR name;
    if ((name = getfilename(obj, title, TRUE)) && *name)
    {
        DoMethod(data->PA_Screens, MUIM_ScreenList_Save, name);
    }

    return 0;
}

/****************************************************************************************/

IPTR MainWindow_Restore(struct IClass *cl, Object *obj, struct MUIP_MainWindow_Restore *msg)
{
    struct MainWindow_Data *data = INST_DATA(cl, obj);
    DoMethod(data->PA_Screens, MUIM_ScreenPanel_CloseWindows);
    DoMethod(data->PA_Screens, MUIM_ScreenList_Load, msg->envarc ? PSD_FILENAME_SAVE : PSD_FILENAME_USE, TRUE);

    return 0;
}

/****************************************************************************************/

IPTR MainWindow_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    Object *BT_Save;
    Object *BT_Use;
    Object *BT_Cancel;
    Object *PA_Screens;
    Object *strip;

    if ((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
            MUIA_Window_Title, "PSI - Public Screen Inspector",
            MUIA_Window_ID   , MAKE_ID('M','A','I','N'),
            MUIA_Window_Menustrip, strip = MUI_MakeObject(MUIO_MenustripNM, MainMenu, 0),
            WindowContents, VGroup,
                Child, PA_Screens = NewObject(CL_ScreenPanel->mcc_Class, NULL, TAG_DONE),
                Child, MUI_MakeObject(MUIO_HBar, 2),
                Child, HGroup, MUIA_Group_SameSize, TRUE,
                    Child, BT_Save = MakeButton(MSG_BUTTON_SAVE),
                    Child, HSpace(0),
                    Child, BT_Use = MakeButton(MSG_BUTTON_USE),
                    Child, HSpace(0),
                    Child, BT_Cancel = MakeButton(MSG_BUTTON_CANCEL),
                End,
            End,
            TAG_MORE, msg->ops_AttrList)))
    {
        struct MainWindow_Data *data = INST_DATA(cl, obj);

        data->PA_Screens = PA_Screens;

        DoMethod(obj      , MUIM_Notify, MUIA_Window_CloseRequest, TRUE , obj, 2, MUIM_MainWindow_Finish, 0);
        DoMethod(BT_Cancel, MUIM_Notify, MUIA_Pressed            , FALSE, obj, 2, MUIM_MainWindow_Finish, 0);
        DoMethod(BT_Use   , MUIM_Notify, MUIA_Pressed            , FALSE, obj, 2, MUIM_MainWindow_Finish, 1);
        DoMethod(BT_Save  , MUIM_Notify, MUIA_Pressed            , FALSE, obj, 2, MUIM_MainWindow_Finish, 2);

        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_ABOUT),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, MUIM_MainWindow_About
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_OPEN),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_MainWindow_Open, 0
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_APPEND),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_MainWindow_Open, 1
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_SAVEAS),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, MUIM_MainWindow_SaveAs
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_QUIT),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_MainWindow_Finish, 0
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_LASTSAVED),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_MainWindow_Restore, 1
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_RESTORE),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 2, MUIM_MainWindow_Restore, 0
        );
        DoMethod
        (
            (Object *)DoMethod(strip, MUIM_FindUData, MEN_MUI),
            MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, MUIV_Notify_Application,
            2, MUIM_Application_OpenConfigWindow, 0
        );

        DoMethod(PA_Screens, MUIM_ScreenList_Load, PSD_FILENAME_USE, TRUE);

        set(BT_Save  , MUIA_ShortHelp, GetStr(MSG_HELP_SAVE  ));
        set(BT_Use   , MUIA_ShortHelp, GetStr(MSG_HELP_USE   ));
        set(BT_Cancel, MUIA_ShortHelp, GetStr(MSG_HELP_CANCEL));

        return (IPTR)obj;
    }

    return 0;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, MainWindow_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW                 : return MainWindow_New    (cl,obj,(APTR)msg);
        case MUIM_MainWindow_Finish : return MainWindow_Finish (cl,obj,(APTR)msg);
        case MUIM_MainWindow_About  : return MainWindow_About  (cl,obj,(APTR)msg);
        case MUIM_MainWindow_Open   : return MainWindow_Open   (cl,obj,(APTR)msg);
        case MUIM_MainWindow_SaveAs : return MainWindow_SaveAs (cl,obj,(APTR)msg);
        case MUIM_MainWindow_Restore: return MainWindow_Restore(cl,obj,(APTR)msg);

        case MUIM_ScreenPanel_CloseWindows:
        case MUIM_ScreenPanel_Update:
        case MUIM_ScreenList_Find:
        {
            struct MainWindow_Data *data = INST_DATA(cl, obj);
            return DoMethodA(data->PA_Screens, msg);
        }
    }

    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID MainWindow_Init(VOID)
{
    CL_MainWindow = MUI_CreateCustomClass
    (
        NULL, MUIC_Window, NULL, sizeof(struct MainWindow_Data), MainWindow_Dispatcher
    );
}

/****************************************************************************************/

VOID MainWindow_Exit(VOID)
{
    if (CL_MainWindow )
        MUI_DeleteCustomClass(CL_MainWindow);
}
