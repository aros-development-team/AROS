/*
    Copyright © 1995-1997 Stefan Stuntz.
    Copyright © 2009-2010, The AROS Development Team.
    All rights reserved.

    $Id$
*/

#include <proto/muimaster.h>
#include <proto/intuition.h>

#include "syspenfield_class.h"

/****************************************************************************************/

struct SysPenField_Data
{
    LONG dummy;
};

/****************************************************************************************/

IPTR SysPenField_DragQuery(struct IClass *cl, Object *obj, struct MUIP_DragQuery *msg)
{
    if (msg->obj==obj)
        return MUIV_DragQuery_Refuse;

    if (muiUserData(msg->obj) < 1 || muiUserData(msg->obj) > 8)
        return MUIV_DragQuery_Refuse;

    return MUIV_DragQuery_Accept;
}

/****************************************************************************************/

IPTR SysPenField_DragDrop(struct IClass *cl, Object *obj, struct MUIP_DragDrop *msg)
{
    set(obj,MUIA_Pendisplay_Reference,msg->obj);
    return 0;
}

/****************************************************************************************/

BOOPSI_DISPATCHER(IPTR, SysPenField_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
        case MUIM_DragQuery  : return(SysPenField_DragQuery  (cl,obj,(APTR)msg));
        case MUIM_DragDrop   : return(SysPenField_DragDrop   (cl,obj,(APTR)msg));
    }
    return DoSuperMethodA(cl,obj,msg);
}
BOOPSI_DISPATCHER_END

/****************************************************************************************/

Object *MakePalette(void)
{
    Object *obj;

    obj = MUI_NewObject(MUIC_Poppen,
        MUIA_CycleChain, 1,
        MUIA_Window_Title, "Adjust Color",
        MUIA_Penadjust_PSIMode, 2,
        MUIA_MaxHeight, 20,
    TAG_DONE);

    return obj;
}

/****************************************************************************************/

Object *MakeMUIPen(int nr, Object **adr)
{
    Object *obj;

    obj = VGroup, MUIA_Group_Spacing, 1,
        Child, *adr = MUI_NewObject(MUIC_Poppen,
            MUIA_CycleChain, 1,
            MUIA_Window_Title, "Adjust MUI Pen",
            MUIA_Penadjust_PSIMode, 1,
            MUIA_MaxHeight, 20,
        TAG_DONE),
        Child, TextObject,
            MUIA_Text_Contents, GetStr(nr),
            MUIA_Text_PreParse, "\33c",
            MUIA_Font, MUIV_Font_Tiny,
        End,
    End;

    return obj;
}

/****************************************************************************************/

Object *MakeSysPen(int nr, Object **adr)
{
    Object *obj;

    obj = VGroup, MUIA_Group_Spacing, 1,
        Child, *adr = NewObject(CL_SysPenField->mcc_Class,NULL,
            TextFrame,
            MUIA_Background, MUII_BACKGROUND,
            MUIA_InnerLeft  , 4,
            MUIA_InnerRight , 4,
            MUIA_InnerTop   , 4,
            MUIA_InnerBottom, 4,
        TAG_DONE),
        Child, TextObject,
            MUIA_Font, MUIV_Font_Tiny,
            MUIA_Text_Contents, GetStr(nr),
            MUIA_Text_PreParse, "\33c",
        End,
    End;

    return obj;
}

/****************************************************************************************/

VOID SysPenField_Init(VOID)
{
    CL_SysPenField = MUI_CreateCustomClass
    (
        NULL, MUIC_Pendisplay, NULL,
        sizeof(struct SysPenField_Data ), SysPenField_Dispatcher
    );
}

/****************************************************************************************/

VOID SysPenField_Exit(VOID)
{
    if (CL_SysPenField )
        MUI_DeleteCustomClass(CL_SysPenField );
}
