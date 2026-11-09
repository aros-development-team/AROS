/*
    Copyright (C) 2013-2026, The AROS Development Team.
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <hidd/hidd.h>
#include <resources/hpet.h>
#include <libraries/mui.h>
#include <mui/NFloattext_mcc.h>
#include <resources/processor.h>
#include <utility/tagitem.h>
#include <utility/hooks.h>

#include <proto/alib.h>
#include <proto/aros.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/hpet.h>
#include <proto/kernel.h>
#include <proto/muimaster.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/processor.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "classes.h"
#include "locale.h"

#include <aros/debug.h>

#include <zune/customclasses.h>

extern OOP_AttrBase HiddAttrBase;
extern OOP_AttrBase HWAttrBase;
extern OOP_MethodID HiddBase;

/*** Instance Data **********************************************************/
struct DevicePage_DATA
{
    /* Nothing to add here */
};


static CONST_STRPTR RelationName(ULONG type)
{
    switch (type)
    {
    case vHidd_Relation_Parent:         return _(MSG_REL_PARENT);
    case vHidd_Relation_Child:          return _(MSG_REL_CHILD);
    case vHidd_Relation_Device:         return _(MSG_REL_DEVICE);
    case vHidd_Relation_Driver:         return _(MSG_REL_DRIVER);
    case vHidd_Relation_Uses:           return _(MSG_REL_USES);
    case vHidd_Relation_UsedBy:         return _(MSG_REL_USEDBY);
    case vHidd_Relation_Exposes:        return _(MSG_REL_EXPOSES);
    case vHidd_Relation_ExposedBy:      return _(MSG_REL_EXPOSEDBY);
    case vHidd_Relation_Firmware:       return _(MSG_REL_FIRMWARE);
    case vHidd_Relation_FirmwareOf:     return _(MSG_REL_FIRMWAREOF);
    case vHidd_Relation_PoweredBy:      return _(MSG_REL_POWEREDBY);
    case vHidd_Relation_Powers:         return _(MSG_REL_POWERS);
    case vHidd_Relation_Monitors:       return _(MSG_REL_MONITORS);
    case vHidd_Relation_MonitoredBy:    return _(MSG_REL_MONITOREDBY);
    case vHidd_Relation_Companion:      return _(MSG_REL_COMPANION);
    }
    return _(MSG_REL_OTHER);
}

/*
 * Adds one line per relation to the group passed as hook message. The
 * subsystem the object is registered in, and the objects registered in it,
 * are what the tree shows already, so they are left out here.
 */
AROS_UFH3S(BOOL, relationFunc,
    AROS_UFHA(struct Hook *, h, A0),
    AROS_UFHA(struct HIDD_Relation *, rel, A2),
    AROS_UFHA(Object **, group, A1))
{
    AROS_USERFUNC_INIT

    CONST_STRPTR name = NULL;
    TEXT buffer[256];

    if ((rel->hr_Type == vHidd_Relation_Parent) || (rel->hr_Type == vHidd_Relation_Child))
        return FALSE;

    if (rel->hr_Object)
    {
        OOP_GetAttr(rel->hr_Object, aHW_ClassName, (IPTR *)&name);
        if (!name)
            OOP_GetAttr(rel->hr_Object, aHidd_HardwareName, (IPTR *)&name);
        if (!name)
            OOP_GetAttr(rel->hr_Object, aHidd_Name, (IPTR *)&name);
    }

    if (name && rel->hr_Path)
        snprintf(buffer, sizeof(buffer), "%s (%s)", name, rel->hr_Path);
    else
        snprintf(buffer, sizeof(buffer), "%s", name ? name : rel->hr_Path ? rel->hr_Path : (CONST_STRPTR)"?");
    if (rel->hr_Label)
    {
        size_t len = strlen(buffer);

        snprintf(buffer + len, sizeof(buffer) - len, " - %s", rel->hr_Label);
    }

    if (!*group)
    {
        *group = ColGroup(2),
            MUIA_FrameTitle, __(MSG_RELATIONS),
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
        End;
    }
    if (*group)
    {
        Object *label = Label(RelationName(rel->hr_Type));
        Object *text = TextObject,
            TextFrame,
            MUIA_Background, MUII_TextBack,
            MUIA_CycleChain, 1,
            MUIA_Text_Contents, (IPTR)buffer,
        End;

        if (label && text)
        {
            DoMethod(*group, OM_ADDMEMBER, label);
            DoMethod(*group, OM_ADDMEMBER, text);
        }
        else
        {
            if (label) MUI_DisposeObject(label);
            if (text) MUI_DisposeObject(text);
        }
    }

    return FALSE;

    AROS_USERFUNC_EXIT
}

static Object *DevicePage__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *device_obj = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    struct Hook relationHook = { .h_Entry = (APTR)relationFunc };
    Object *relations = NULL;
    IPTR devicePath = 0;
    IPTR idName, hwName, vendorStr;
    IPTR prodVal, vendVal;
    TEXT productId[20], vendorId[20];

    OOP_GetAttr(device_obj, aHidd_Name, &idName);
    OOP_GetAttr(device_obj, aHidd_HardwareName, &hwName);
    OOP_GetAttr(device_obj, aHidd_ProducerName, &vendorStr);
    OOP_GetAttr(device_obj, aHidd_Product, &prodVal);
    sprintf(productId, "0x%04lX", prodVal);
    OOP_GetAttr(device_obj, aHidd_Producer, &vendVal);
    sprintf(vendorId, "0x%04lX", vendVal);
    OOP_GetAttr(device_obj, aHidd_DevicePath, &devicePath);

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,

        Child, (IPTR)(ColGroup(2),
            MUIA_FrameTitle, __(MSG_GENERAL),
            GroupFrame,
            MUIA_Background, MUII_GroupBack,
            Child, (IPTR)Label(_(MSG_NAME)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, idName,
            End),
            Child, (IPTR)Label(_(MSG_HARDWARE_NAME)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, hwName,
            End),
            (vendVal != 0) ? Child : TAG_IGNORE, (IPTR)Label(_(MSG_PRODUCT_ID)),
            (vendVal != 0) ? Child : TAG_IGNORE, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)productId,
            End),
            Child, (IPTR)Label(_(MSG_PRODUCER_NAME)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, vendorStr,
            End),
            (vendVal != 0) ? Child : TAG_IGNORE, (IPTR)Label(_(MSG_PRODUCER_ID)),
            (vendVal != 0) ? Child : TAG_IGNORE, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)vendorId,
            End),
            (devicePath != 0) ? Child : TAG_IGNORE, (IPTR)Label(_(MSG_DEVICE_PATH)),
            (devicePath != 0) ? Child : TAG_IGNORE, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, devicePath,
            End),
        End),
        TAG_DONE
    );

    if (self)
    {
        OOP_Class *ocl;

        /* Method IDs only mean something to the classes which have the interface */
        for (ocl = OOP_OCLASS(device_obj); ocl; ocl = ocl->superclass)
        {
            if (ocl->ClassNode.ln_Name && !strcmp(ocl->ClassNode.ln_Name, CLID_Hidd))
            {
                HIDD_EnumRelations(device_obj, vHidd_Relation_Any, &relationHook, &relations);
                break;
            }
        }
        if (relations)
            DoMethod(self, OM_ADDMEMBER, relations);
    }

    return self;
}


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    DevicePage, NULL, MUIC_Group, NULL,
    OM_NEW, struct opSet *
);
