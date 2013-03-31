/*
    Copyright (C) 2013, The AROS Development Team.
    $Id$
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

#include "classes.h"
#include "locale.h"

#include <aros/debug.h>

#include <zune/customclasses.h>


/*** Instance Data **********************************************************/
struct DevicePage_DATA
{
    /* Nothing to add here */
};


static Object *DevicePage__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    OOP_Object *device_obj = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
    IPTR idName, hwName, vendorStr;
    IPTR number;
    TEXT productId[20], vendorId[20];

    OOP_GetAttr(device_obj, aHidd_Name, &idName);
    OOP_GetAttr(device_obj, aHidd_HardwareName, &hwName);
    OOP_GetAttr(device_obj, aHidd_ProducerName, &vendorStr);
    OOP_GetAttr(device_obj, aHidd_Product, &number);
    sprintf(productId, "0x%04lX", number);
    OOP_GetAttr(device_obj, aHidd_Producer, &number);
    sprintf(vendorId, "0x%04lX", number);

    return (Object *) DoSuperNewTags
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
            Child, (IPTR)Label(_(MSG_PRODUCT_ID)),
            Child, (IPTR)(TextObject,
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
            Child, (IPTR)Label(_(MSG_PRODUCER_ID)),
            Child, (IPTR)(TextObject,
                TextFrame,
                MUIA_Background, MUII_TextBack,
                MUIA_CycleChain, 1,
                MUIA_Text_Contents, (IPTR)vendorId,
            End),
        End),
        TAG_DONE
    );
}


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    DevicePage, NULL, MUIC_Group, NULL,
    OM_NEW, struct opSet *
);
