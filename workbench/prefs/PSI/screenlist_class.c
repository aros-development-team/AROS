/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2011, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/muiscreen.h>
#include <proto/graphics.h>
#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include <datatypes/pictureclass.h>

#include <string.h>
#include <stdio.h>

#include "screenlist_class.h"

#define USE_PSI_SCREENON_HEADER
#define USE_PSI_SCREENON_BODY
#define USE_PSI_SCREENON_COLORS
#include "psi_screenon.bh"

/*
#define USE_PSI_SCREENOF_BODY
#include "psi_screenof.bh"
*/

#define USE_PSI_SCREENCN_BODY
#include "psi_screencn.bh"

/*
#define USE_PSI_SCREENCF_BODY
#include "psi_screencf.bh"
*/

/****************************************************************************************/

struct ScreenList_Data
{
    Object *list;
    Object *onormal[2];
    /*
    Object *oforeign[2];
    */
    APTR inormal[2];
    /*
    APTR iforeign[2];
    */
    struct Hook DisplayHook;
};

/****************************************************************************************/

IPTR ScreenList_ConstructFunc(struct Hook *hook, APTR pool, struct MUI_PubScreenDesc *src)
{
    struct MUI_PubScreenDesc *desc;

    if ((desc = MUIS_AllocPubScreenDesc(src)))
    {
        desc->Changed  = FALSE;
        desc->UserData = NULL;
    }

    return (IPTR)desc;
}

/****************************************************************************************/

VOID ScreenList_DestructFunc(struct Hook *hook, APTR pool, struct MUI_PubScreenDesc *desc)
{
    MUIS_FreePubScreenDesc(desc);
}

/****************************************************************************************/

LONG ScreenList_CompareFunc(struct Hook *hook, struct MUI_PubScreenDesc *d2, struct MUI_PubScreenDesc *d1)
{
    if (!strcmp(d1->Name, PSD_INITIAL_NAME))
        return strcmp(d2->Name, PSD_INITIAL_NAME) ? 1 : 0;
    else if (!strcmp(d2->Name, PSD_INITIAL_NAME))
        return -1;
    else
        return stricmp(d1->Name, d2->Name);
}

/****************************************************************************************/

LONG ScreenList_DisplayFunc(struct Hook *hook, char **array, struct MUI_PubScreenDesc *desc)
{
    struct ScreenList_Data *data = (APTR)hook->h_Data;

    *array++ = "";

    if (!desc)
    {
        static char buf1[30], buf2[30];
        strcpy(buf1, "\33b\33u");
        strcpy(buf2, "\33b\33u");
        strcat(buf1, GetStr(MSG_LIST_SCREENNAME));
        strcat(buf2, GetStr(MSG_LIST_SCREENMODE));
        *array++ = "";
        *array++ = buf1;
        *array   = buf2;
    }
    else
    {
        static struct NameInfo ni;
        static char buf1[PSD_MAXLEN_NAME+2];
        static char buf2[50];

        strcpy(buf1, desc->UserData ? "\33b" : desc->Changed ? "\33u" : "");
        strcat(buf1, desc->Name);

        /*
        if (desc->Foreign)
        {
            strcpy(ni.Name,GetStr(MSG_LIST_FOREIGNSCREEN));
            sprintf(buf2,"\33O[%08lx]",data->iforeign[TestPubScreen(desc->Name) ? 1 : 0]);
        }
        else
        */
        {
            if (!GetDisplayInfoData(0, (UBYTE *)&ni, sizeof(ni), DTAG_NAME, desc->DisplayID))
                strcpy(ni.Name, GetStr(MSG_LIST_UNKNOWNMODE));

            sprintf(buf2, "\33O[%08lx]", data->inormal[TestPubScreen(desc->Name) ? 1 : 0]);
        }

        *array++ = buf2;
        *array++ = buf1;
        *array   = ni.Name;
    }

    return 0;
}

/****************************************************************************************/

IPTR ScreenList_Load(struct IClass *cl, Object *obj, struct MUIP_ScreenList_Load *msg)
{
    IPTR result = FALSE;
    struct MUI_PubScreenDesc *desc;
    APTR pfh;

    if ((pfh = MUIS_OpenPubFile(msg->name,MODE_OLDFILE)))
    {
        result = TRUE;

        if (msg->clear)
            DoMethod(obj, MUIM_List_Clear);

        set(obj, MUIA_List_Quiet, TRUE);

        while ((desc = MUIS_ReadPubFile(pfh)))
        {
            DoMethod(obj, MUIM_List_InsertSingle, desc, MUIV_List_Insert_Sorted);
        }

        set(obj, MUIA_List_Quiet, FALSE);

        MUIS_ClosePubFile(pfh);
    }

    return result;
}

/****************************************************************************************/

IPTR ScreenList_Save(struct IClass *cl, Object *obj, struct MUIP_ScreenList_Save *msg)
{
    IPTR result = FALSE;
    struct MUI_PubScreenDesc *desc;
    APTR pfh;
    int i;

    DoMethod(obj, MUIM_List_Sort);

    if ((pfh = MUIS_OpenPubFile(msg->name, MODE_NEWFILE)))
    {
        result = TRUE;

        for (i=0; result; i++)
        {
            DoMethod(obj, MUIM_List_GetEntry, i, &desc);
            if (!desc)
                break;

            desc->Changed  = FALSE;
            desc->UserData = NULL;

            if (!MUIS_WritePubFile(pfh, desc))
                result = FALSE;
        }
        MUIS_ClosePubFile(pfh);
    }

    return result;
}

/****************************************************************************************/

IPTR ScreenList_Find(struct IClass *cl, Object *obj, struct MUIP_ScreenList_Find *msg)
{
    int i;
    struct MUI_PubScreenDesc *desc;

    *(msg->desc) = NULL;

    for (i = 0; ; i++)
    {
        DoMethod(obj, MUIM_List_GetEntry, i, &desc);
        if (!desc)
            break;
        if (!stricmp(desc->Name, msg->name))
        {
            *(msg->desc) = desc;
            set(obj, MUIA_List_Active, i);
            break;
        }
    }

    return 0;
}

/****************************************************************************************/

static Object *makescreenimage(UBYTE *body)
{
    Object *obj = BodychunkObject,
        MUIA_FixWidth             , PSI_SCREENON_WIDTH ,
        MUIA_FixHeight            , PSI_SCREENON_HEIGHT,
        MUIA_Bitmap_Width         , PSI_SCREENON_WIDTH ,
        MUIA_Bitmap_Height        , PSI_SCREENON_HEIGHT,
        MUIA_Bodychunk_Depth      , PSI_SCREENON_DEPTH ,
        MUIA_Bodychunk_Body       , (UBYTE *)body,
        MUIA_Bodychunk_Compression, PSI_SCREENON_COMPRESSION,
        MUIA_Bodychunk_Masking    , PSI_SCREENON_MASKING,
        MUIA_Bitmap_SourceColors  , (ULONG *)psi_screenon_colors,
        MUIA_Bitmap_Transparent   , 0,
    End;

    return obj;
}

/****************************************************************************************/

IPTR ScreenList_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenList_Data *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, msg))
        return FALSE;

    data->onormal[0] = makescreenimage((UBYTE *)psi_screencn_body);
    data->onormal[1] = makescreenimage((UBYTE *)psi_screenon_body);
    /*
    data->oforeign[0] = makescreenimage((UBYTE *)psi_screencf_body);
    data->oforeign[1] = makescreenimage((UBYTE *)psi_screenof_body);
    */

    data->inormal[0] = (APTR)DoMethod(obj, MUIM_List_CreateImage, data->onormal[0], 0);
    data->inormal[1] = (APTR)DoMethod(obj, MUIM_List_CreateImage, data->onormal[1], 0);
    /*
    data->iforeign[0] = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->oforeign[0],0);
    data->iforeign[1] = (APTR)DoMethod(obj,MUIM_List_CreateImage,data->oforeign[1],0);
    */

    MUI_RequestIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY);

    return TRUE;
}

/****************************************************************************************/

IPTR ScreenList_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct ScreenList_Data *data = INST_DATA(cl, obj);

    DoMethod(obj, MUIM_List_DeleteImage, data->inormal[0]);
    DoMethod(obj, MUIM_List_DeleteImage, data->inormal[1]);
    /*
    DoMethod(obj,MUIM_List_DeleteImage,data->iforeign[0]);
    DoMethod(obj,MUIM_List_DeleteImage,data->iforeign[1]);
    */

    if (data->onormal[0])
        MUI_DisposeObject(data->onormal[0]);
    if (data->onormal[1])
        MUI_DisposeObject(data->onormal[1]);
    /*
    if (data->oforeign[0]) MUI_DisposeObject(data->oforeign[0]);
    if (data->oforeign[1]) MUI_DisposeObject(data->oforeign[1]);
    */

    MUI_RejectIDCMP(obj, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY);

    return DoSuperMethodA(cl, obj, msg);
}

/****************************************************************************************/

IPTR ScreenList_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    static struct Hook ScreenList_ConstructHook;
    ScreenList_ConstructHook.h_Entry = HookEntry;
    ScreenList_ConstructHook.h_SubEntry = ScreenList_ConstructFunc;
    static struct Hook ScreenList_DestructHook;
    ScreenList_DestructHook.h_Entry = HookEntry;
    ScreenList_DestructHook.h_SubEntry = (HOOKFUNC)ScreenList_DestructFunc;
    static struct Hook ScreenList_CompareHook;
    ScreenList_CompareHook.h_Entry = HookEntry;
    ScreenList_CompareHook.h_SubEntry = (HOOKFUNC)ScreenList_CompareFunc;

    obj=(Object *)DoSuperNewTags(cl, obj, NULL,
        MUIA_List_ConstructHook, &ScreenList_ConstructHook,
        MUIA_List_DestructHook , &ScreenList_DestructHook,
        MUIA_List_CompareHook  , &ScreenList_CompareHook,
        MUIA_List_Format       , "DELTA=2,,,",
        MUIA_List_Title        , TRUE,
        MUIA_List_MinLineHeight, 14,
        TAG_MORE, msg->ops_AttrList);

    if (obj)
    {
        struct ScreenList_Data *data = INST_DATA(cl, obj);

        data->DisplayHook.h_Entry = HookEntry;
        data->DisplayHook.h_SubEntry = (HOOKFUNC)ScreenList_DisplayFunc;
        data->DisplayHook.h_Data  = (APTR)data;

        set(obj, MUIA_List_DisplayHook, &data->DisplayHook);
    }

    return (IPTR)obj;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, ScreenList_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case OM_NEW              : return ScreenList_New    (cl,obj,(APTR)msg);
        case MUIM_Setup          : return ScreenList_Setup  (cl,obj,(APTR)msg);
        case MUIM_Cleanup        : return ScreenList_Cleanup(cl,obj,(APTR)msg);
        case MUIM_ScreenList_Save: return ScreenList_Save   (cl,obj,(APTR)msg);
        case MUIM_ScreenList_Load: return ScreenList_Load   (cl,obj,(APTR)msg);
        case MUIM_ScreenList_Find: return ScreenList_Find   (cl,obj,(APTR)msg);
    }

    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

VOID ScreenList_Init(VOID)
{
    CL_ScreenList = MUI_CreateCustomClass
    (
        NULL, MUIC_List, NULL, sizeof(struct ScreenList_Data), ScreenList_Dispatcher
    );
}

/****************************************************************************************/

VOID ScreenList_Exit(VOID)
{
    if (CL_ScreenList)
        MUI_DeleteCustomClass(CL_ScreenList);
}
