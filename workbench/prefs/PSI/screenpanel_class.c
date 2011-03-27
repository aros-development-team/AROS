/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/intuition.h>
#include <proto/muiscreen.h>
#include <proto/muimaster.h>

#include <string.h>

#include "screenlist_class.h"
#include "editwindow_class.h"
#include "editpanel_class.h"
#include "screenpanel_class.h"

/****************************************************************************************/

struct ScreenPanel_Data
{
    Object *LV_Screens;
    Object *BT_Create;
    Object *BT_Copy;
    Object *BT_Delete;
    Object *BT_Edit;
    Object *BT_Open;
    Object *BT_Close;
    Object *BT_Jump;
    #ifdef MYDEBUG
    Object *BT_Foo;
    #endif
};

/****************************************************************************************/

IPTR ScreenPanel_Finish(struct IClass *cl, Object *obj, struct MUIP_ScreenPanel_Finish *msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    int i;
    Object *win = msg->win;
    BOOL ok = msg->ok;

    for (i=0; ; i++)
    {
        DoMethod(data->LV_Screens, MUIM_List_GetEntry, i, &desc);
        if (!desc)
            return 0; /* should never happen */
        if (desc->UserData == win)
            break;
    }

    desc->UserData = NULL;

    if (ok)
    {
        DoMethod(win, MUIM_EditPanel_GetScreen, desc);
        desc->Changed = TRUE;
    }
    DoMethod(data->LV_Screens, MUIM_List_Redraw, i);
    DoMethod(obj, MUIM_ScreenPanel_SetStates);

    set(win, MUIA_Window_Open, FALSE);
    DoMethod((Object *)xget(obj, MUIA_ApplicationObject), OM_REMMEMBER, win);
    MUI_DisposeObject(win);

    return 0;
}
/****************************************************************************************/

IPTR ScreenPanel_Edit(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;

    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);

    if (desc)
    {
        set((Object *)xget(obj, MUIA_ApplicationObject), MUIA_Application_Sleep, TRUE);

        if (!desc->UserData)
        {
            if ((desc->UserData = (APTR)NewObject(CL_EditWindow->mcc_Class, NULL,
                MUIA_Window_Width , MUIV_Window_Width_MinMax(0),
                MUIA_Window_Height, MUIV_Window_Height_MinMax(0),
                MUIA_EditWindow_Title, desc->Name,
                MUIA_EditWindow_Originator, obj,
                TAG_DONE)))
            {
                DoMethod((Object *)xget(obj, MUIA_ApplicationObject), OM_ADDMEMBER, desc->UserData);
                DoMethod(desc->UserData,MUIM_EditPanel_SetScreen, desc);
                DoMethod(data->LV_Screens, MUIM_List_Redraw, MUIV_List_Redraw_Active);
            }
        }

        if (desc->UserData)
            set(desc->UserData, MUIA_Window_Open, TRUE);
        else
            DisplayBeep(0);

        set((Object *)xget(obj, MUIA_ApplicationObject), MUIA_Application_Sleep, FALSE);
    }

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Delete(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;

    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);
    if (desc)
    {
        if (!desc->UserData)
        {
            MUIS_ClosePubScreen(desc->Name);
            DoMethod(data->LV_Screens, MUIM_List_Remove, MUIV_List_Remove_Active);
        }
        else
            DisplayBeep(0);
    }

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Create(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    DoMethod(data->LV_Screens, MUIM_List_InsertSingle, NULL, MUIV_List_Insert_Bottom);
    set(data->LV_Screens, MUIA_List_Active, MUIV_List_Active_Bottom);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Copy(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *new,*src;
    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &src);
    if ((new = MUIS_AllocPubScreenDesc(src)))
    {
        char namebuf[PSD_MAXLEN_NAME];
        strcpy(namebuf, new->Name);
        strcpy(new->Name, ">");
        stccpy(new->Name + 1, namebuf, PSD_MAXLEN_NAME - 1);
        DoMethod(data->LV_Screens, MUIM_List_InsertSingle, new, MUIV_List_Insert_Bottom);
        set(data->LV_Screens, MUIA_List_Active, MUIV_List_Active_Bottom);
        MUIS_FreePubScreenDesc(new);
    }
    else
        DisplayBeep(0);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_SetStates(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);
    if (desc)
    {
        /*
        if (desc->Foreign)
        {
            set(data->BT_Copy  , MUIA_Disabled, FALSE);
            set(data->BT_Delete, MUIA_Disabled, FALSE);
            set(data->BT_Edit  , MUIA_Disabled, FALSE);
            set(data->BT_Open  , MUIA_Disabled, TRUE );
            set(data->BT_Close , MUIA_Disabled, TRUE );
            set(data->BT_Jump  , MUIA_Disabled, FALSE);
        }
        else
        */
        {
            BOOL opened = TestPubScreen(desc->Name);

            set(data->BT_Copy  , MUIA_Disabled, FALSE);
            set(data->BT_Delete, MUIA_Disabled, FALSE);
            set(data->BT_Edit  , MUIA_Disabled, FALSE);
            set(data->BT_Open  , MUIA_Disabled, opened);
            set(data->BT_Close , MUIA_Disabled, !opened);
            set(data->BT_Jump  , MUIA_Disabled, FALSE);
        }
    }
    else
    {
        set(data->BT_Copy  , MUIA_Disabled, TRUE);
        set(data->BT_Delete, MUIA_Disabled, TRUE);
        set(data->BT_Edit  , MUIA_Disabled, TRUE);
        set(data->BT_Open  , MUIA_Disabled, TRUE);
        set(data->BT_Close , MUIA_Disabled, TRUE);
        set(data->BT_Jump  , MUIA_Disabled, TRUE);
    }

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Close(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);
    if (!desc || !MUIS_ClosePubScreen(desc->Name))
        DisplayBeep(0);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Open(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);
    if (desc)
    {
        if (desc->Changed)
        {
            /* !!! */
        }
        if (!MUIS_OpenPubScreen(desc))
            DisplayBeep(0);
    }
    else
        DisplayBeep(0);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Jump(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl,obj);
    struct MUI_PubScreenDesc *desc;
    DoMethod(data->LV_Screens, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &desc);
    if (desc)
    {
        if (desc->Changed)
        {
            /* !!! */
        }
        DoMethod
        (
            (Object *)xget(obj, MUIA_ApplicationObject), MUIM_Application_SetConfigItem,
            MUICFG_PublicScreen, desc->Name
        );
    }
    else
        DisplayBeep(0);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_Update(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    DoMethod(data->LV_Screens, MUIM_List_Redraw, MUIV_List_Redraw_All);
    DoMethod(obj, MUIM_ScreenPanel_SetStates);

    return 0;
}

/****************************************************************************************/

IPTR ScreenPanel_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct ScreenPanel_Data tmp = {0};

    obj = (Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_Group_Horiz, FALSE,
        MUIA_Group_VertSpacing, 0,
        Child, tmp.LV_Screens = ListviewObject,
            MUIA_CycleChain, 1,
            MUIA_Listview_List, NewObject
            (
                CL_ScreenList->mcc_Class, NULL, InputListFrame,
                MUIA_List_AutoVisible, TRUE,
                TAG_DONE
            ),
        End,
        Child, ColGroup(4), GroupSpacing(0), MUIA_Group_SameSize, TRUE,
            Child, tmp.BT_Create = MakeButton(MSG_BUTTON_NEW),
            Child, tmp.BT_Copy   = MakeButton(MSG_BUTTON_COPY),
            Child, tmp.BT_Delete = MakeButton(MSG_BUTTON_DELETE),
            Child, tmp.BT_Edit   = MakeButton(MSG_BUTTON_EDIT),
            Child, tmp.BT_Open   = MakeButton(MSG_BUTTON_OPEN),
            Child, tmp.BT_Close  = MakeButton(MSG_BUTTON_CLOSE),
            Child, tmp.BT_Jump   = MakeButton(MSG_BUTTON_JUMP),
            #ifdef MYDEBUG
            Child, tmp.BT_Foo    = SimpleButton("Foo"),
            #else
            Child, HVSpace,
            #endif
        End,
    TAG_DONE);

    if (obj)
    {
        struct ScreenPanel_Data *data = INST_DATA(cl, obj);

        *data = tmp;

        DoMethod(tmp.BT_Delete, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Delete);
        DoMethod(tmp.BT_Create, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Create);
        DoMethod(tmp.BT_Copy  , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Copy  );
        DoMethod(tmp.BT_Edit  , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Edit  );
        DoMethod(tmp.BT_Open  , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Open  );
        DoMethod(tmp.BT_Close , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Close );
        DoMethod(tmp.BT_Jump  , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Jump  );
        #ifdef MYDEBUG
        DoMethod(tmp.BT_Foo   , MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MUIM_ScreenPanel_Foo   );
        #endif
        DoMethod(tmp.LV_Screens, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,  obj, 1, MUIM_ScreenPanel_Edit);
        DoMethod(tmp.LV_Screens, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime, obj, 1, MUIM_ScreenPanel_SetStates);

        set(tmp.BT_Delete , MUIA_ShortHelp,GetStr(MSG_HELP_DELETESCREEN));
        set(tmp.BT_Create , MUIA_ShortHelp,GetStr(MSG_HELP_NEWSCREEN   ));
        set(tmp.BT_Copy   , MUIA_ShortHelp,GetStr(MSG_HELP_COPYSCREEN  ));
        set(tmp.BT_Edit   , MUIA_ShortHelp,GetStr(MSG_HELP_EDITSCREEN  ));
        set(tmp.BT_Open   , MUIA_ShortHelp,GetStr(MSG_HELP_OPENSCREEN  ));
        set(tmp.BT_Close  , MUIA_ShortHelp,GetStr(MSG_HELP_CLOSESCREEN ));
        set(tmp.BT_Jump   , MUIA_ShortHelp,GetStr(MSG_HELP_JUMPSCREEN  ));
        set(tmp.LV_Screens, MUIA_ShortHelp,GetStr(MSG_HELP_SCREENLIST  ));

        DoMethod(obj, MUIM_ScreenPanel_SetStates);
    }

    return (IPTR)obj;
}

/****************************************************************************************/

IPTR ScreenPanel_Dispose(struct IClass *cl, Object *obj, Msg msg)
{
    DoMethod(obj, MUIM_ScreenPanel_CloseWindows);
    return(DoSuperMethodA(cl, obj, msg));
}
/****************************************************************************************/

IPTR ScreenPanel_CloseWindows(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    int i;
    for (i=0; ; i++)
    {
        DoMethod(data->LV_Screens, MUIM_List_GetEntry, i, &desc);
        if (!desc)
            break;
        if (desc->UserData)
            DoMethod(obj, MUIM_ScreenPanel_Finish, desc->UserData, FALSE);
    }

    return 0;
}
/****************************************************************************************/

#ifdef MYDEBUG

BOOL SavePubScreen(struct MUI_PubScreenDesc *desc)
{
    BPTR file;
    char filename[PSD_MAXLEN_NAME+16];
    int p;
    BOOL rc = FALSE;

    strcpy(filename, "mui:Screens");
    AddPart(filename, desc->Name, sizeof(filename));
    strcat(filename, ".mps");

    if (file = Open(filename,MODE_NEWFILE))
    {
        FPrintf(file, "T=\"%s\"\n", desc->Title       );
        FPrintf(file, "F=\"%s\"\n", desc->Font        );
        FPrintf(file, "B=\"%s\"\n", desc->Background  );
        FPrintf(file, "W=%ld\n"   , desc->DisplayWidth);
        FPrintf(file, "H=%ld\n"   , desc->DisplayHeight);
        FPrintf(file, "D=%ld\n"   , desc->DisplayDepth);
        FPrintf(file, "I=%ld\n"   , desc->DisplayID   );
        if (desc->OverscanType) FPrintf(file, "OS\n");
        if (desc->AutoScroll  ) FPrintf(file, "AS\n");
        if (desc->NoDrag      ) FPrintf(file, "ND\n");
        if (desc->Exclusive   ) FPrintf(file, "EX\n");
        if (desc->Interleaved ) FPrintf(file, "IN\n");
        if (desc->SysDefault  ) FPrintf(file, "SD\n");
        if (desc->Behind      ) FPrintf(file, "BH\n");
        if (desc->AutoClose   ) FPrintf(file, "AC\n");
        if (desc->CloseGadget ) FPrintf(file, "CG\n");

        FPrintf(file, "PEN=\"");
        for (p = 0; p < PSD_NUMSYSPENS; p++)
            FPrintf(file, "%ld:%ld ", p, desc->SystemPens[p]);
        FPrintf(file, "\"\n");

        FPrintf(file, "PAL=\"");
        for (p = 0; p < PSD_NUMCOLS; p++)
            FPrintf
            (
                file, "%ld:%02lx%02lx%02lx ", p < 4 ? p : p - 8,
                desc->Palette[p].red >> 24,
                desc->Palette[p].green >> 24,
                desc->Palette[p].blue >> 24
            );
        FPrintf(file, "\"\n");

        rc = TRUE;

        Close(file);
    }

    return rc;
}

/****************************************************************************************/

IPTR ScreenPanel_Foo(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenPanel_Data *data = INST_DATA(cl, obj);
    struct MUI_PubScreenDesc *desc;
    int i;

    for (i = 0; ; i++)
    {
        int p;

        DoMethod(data->LV_Screens, MUIM_List_GetEntry, i, &desc);
        if (!desc)
            break;

        printf("N=\"%s\"\n", desc->Name        );
        printf("T=\"%s\"\n", desc->Title       );
        printf("F=\"%s\"\n", desc->Font        );
        printf("B=\"%s\"\n", desc->Background  );
        printf("W=%ld\n"   , desc->DisplayWidth);
        printf("H=%ld\n"   , desc->DisplayHeight);
        printf("D=%ld\n"   , desc->DisplayDepth);
        printf("I=%ld\n"   , desc->DisplayID   );
        if (desc->OverscanType) printf("OS\n");
        if (desc->AutoScroll  ) printf("AS\n");
        if (desc->NoDrag      ) printf("ND\n");
        if (desc->Exclusive   ) printf("EX\n");
        if (desc->Interleaved ) printf("IN\n");
        if (desc->SysDefault  ) printf("SD\n");
        if (desc->Behind      ) printf("BH\n");
        if (desc->AutoClose   ) printf("AC\n");
        if (desc->CloseGadget ) printf("CG\n");

        printf("PENS=\"");
        for (p = 0; p < PSD_NUMSYSPENS; p++)
            printf("%ld:%ld ", p, desc->SystemPens[p]);
        printf("\"\n");

        printf("PALETTE=\"");
        for (p = 0; p < PSD_NUMCOLS; p++)
            printf
            (
                "%ld:%02lx%02lx%02lx ", p < 4 ? p : p - 8,
                desc->Palette[p].red >> 24,
                desc->Palette[p].green >> 24,
                desc->Palette[p].blue >> 24
            );
        printf("\"\n");

        printf("\n");

        /*
        if (!desc->Foreign)
        */
            SavePubScreen(desc);
    }

    return 0;
}
#endif

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, ScreenPanel_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW:
            return ScreenPanel_New(cl, obj, (APTR)msg);

        case OM_DISPOSE:
            return ScreenPanel_Dispose(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Create:
            return ScreenPanel_Create(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Copy:
            return ScreenPanel_Copy(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Delete:
            return ScreenPanel_Delete(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Edit:
            return ScreenPanel_Edit(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Finish:
            return ScreenPanel_Finish(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_CloseWindows:
            return ScreenPanel_CloseWindows(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_SetStates:
            return ScreenPanel_SetStates(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Open:
            return ScreenPanel_Open(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Close:
            return ScreenPanel_Close(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Jump:
            return ScreenPanel_Jump(cl, obj, (APTR)msg);

        case MUIM_ScreenPanel_Update:
            return ScreenPanel_Update(cl, obj, (APTR)msg);
        #ifdef MYDEBUG

        case MUIM_ScreenPanel_Foo:
            return ScreenPanel_Foo(cl, obj, (APTR)msg);
        #endif

        case MUIM_ScreenList_Find:
        {
            struct ScreenPanel_Data *data = INST_DATA(cl, obj);
            return(DoMethodA(data->LV_Screens, msg));
        }
    }

    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID ScreenPanel_Init(VOID)
{
    CL_ScreenPanel = MUI_CreateCustomClass
    (
        NULL, MUIC_Group, NULL, sizeof(struct ScreenPanel_Data ), ScreenPanel_Dispatcher
    );
}

VOID ScreenPanel_Exit(VOID)
{
    if (CL_ScreenPanel )
        MUI_DeleteCustomClass(CL_ScreenPanel);
}
