/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/intuition.h>
#include <proto/muimaster.h>
#include <proto/utility.h>

#include <libraries/muiscreen.h>

#include <string.h>

#include "editpanel_class.h"
#include "editwindow_class.h"
#include "screenpanel_class.h"

/****************************************************************************************/

struct EditWindow_Data
{
    Object *panel;
    char wtitle[PSD_MAXLEN_TITLE + 20];
};

/****************************************************************************************/

IPTR EditWindow_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct EditWindow_Data *data;
    Object *ok;
    Object *cancel;
    Object *panel;
    Object *originator;
    /*Object *strip;*/

    if ((obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        /*MUIA_Window_Menustrip, strip = MUI_MakeObject(MUIO_MenustripNM,EditMenu,0),*/
        WindowContents, VGroup,
            Child, panel = NewObject(CL_EditPanel->mcc_Class, NULL, TAG_DONE),
            Child, HGroup, MUIA_Group_SameSize, TRUE,
                Child, ok = MakeButton(MSG_BUTTON_OK),
                Child, HSpace(0),
                Child, HSpace(0),
                Child, HSpace(0),
                Child, cancel = MakeButton(MSG_BUTTON_CANCEL),
            End,
        End,
        TAG_MORE, msg->ops_AttrList)))
    {
        data = INST_DATA(cl, obj);

        data->panel = panel;

        strcpy(data->wtitle, GetStr(MSG_TITLE_PUBSCREENWINDOW));
        strcat(data->wtitle, " ");
        strcat(data->wtitle, (char *)GetTagData(MUIA_EditWindow_Title, (IPTR)"", msg->ops_AttrList));

        set(obj, MUIA_Window_Title, data->wtitle);
        set(obj, MUIA_Window_ID   , MAKE_ID('E','D','I','T'));

        originator = (Object *)GetTagData(MUIA_EditWindow_Originator, 0, msg->ops_AttrList);

        DoMethod
        (
            obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE, MUIV_Notify_Application,
            6, MUIM_Application_PushMethod, originator,
            3, MUIM_ScreenPanel_Finish, obj, FALSE
        );
        DoMethod
        (
            cancel, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application,
            6, MUIM_Application_PushMethod, originator,
            3, MUIM_ScreenPanel_Finish, obj, FALSE
        );
        DoMethod
        (
            ok, MUIM_Notify, MUIA_Pressed, FALSE, MUIV_Notify_Application, 6,
            MUIM_Application_PushMethod, originator, 3, MUIM_ScreenPanel_Finish, obj, TRUE
        );

        /*
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_2COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,0);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_4COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,1);
        DoMethod((Object *)DoMethod(strip,MUIM_FindUData,MEN_8COL),MUIM_Notify,MUIA_Menuitem_Trigger,MUIV_EveryTime,panel,2,MUIM_EditPanel_DefColors,2);
        */

        set(ok, MUIA_ShortHelp, GetStr(MSG_HELP_EDITOK));
        set(cancel, MUIA_ShortHelp, GetStr(MSG_HELP_EDITCANCEL));

        return (IPTR)obj;
    }
    return 0;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, EditWindow_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW: return EditWindow_New(cl, obj, (APTR)msg);

        /*
        ** The next methods actually don't belong to the
        ** edit window class. We just forward them here to
        ** allow treating an edit window much like an edit
        ** panel from outside.
        */

        case MUIM_EditPanel_SetScreen:
        case MUIM_EditPanel_GetScreen:
        {
            struct EditWindow_Data *data = INST_DATA(cl, obj);
            return DoMethodA(data->panel, msg);
        }
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID EditWindow_Init(VOID)
{
    CL_EditWindow = MUI_CreateCustomClass
    (
        NULL, MUIC_Window, NULL, sizeof(struct EditWindow_Data), EditWindow_Dispatcher
    );
}

/****************************************************************************************/

VOID EditWindow_Exit(VOID)
{
    if (CL_EditWindow)
        MUI_DeleteCustomClass(CL_EditWindow);
}
