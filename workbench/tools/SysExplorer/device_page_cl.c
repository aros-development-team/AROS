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

#include "device_page_cl.h"
#include "locale.h"

#define DEBUG 1
#include <aros/debug.h>

#include <zune/customclasses.h>


/*** Instance Data **********************************************************/
struct DevicePage_DATA
{
    Object      *name_txt;
    Object      *hardwarename_txt;
    Object      *product_txt; // numeric
    Object      *producername_txt;
    Object      *producer_txt; // numeric
};


static Object *DevicePage__OM_NEW(Class *cl, Object *self, struct opSet *msg)
{
    Object *name_txt;
    Object *hardwarename_txt;
    Object *product_txt; // numeric
    Object *producername_txt;
    Object *producer_txt; // numeric

    self = (Object *) DoSuperNewTags
    (
        cl, self, NULL,

        Child, (IPTR)(VGroup,
            MUIA_FrameTitle, (IPTR)"Device",
            GroupFrame,
            Child, (IPTR)(ColGroup(2),
                Child, (IPTR)Label("Name"),
                Child, (IPTR)(name_txt = TextObject,
                    TextFrame,
                    MUIA_Text_Contents, (IPTR)"                                 ",
                End),
                Child, (IPTR)Label("Hardware Name"),
                Child, (IPTR)(hardwarename_txt = TextObject,
                    TextFrame,
                End),
                Child, (IPTR)Label("Product ID"),
                Child, (IPTR)(product_txt = TextObject,
                    TextFrame,
                End),
                Child, (IPTR)Label("Producer Name"),
                Child, (IPTR)(producername_txt = TextObject,
                    TextFrame,
                End),
                Child, (IPTR)Label("Producer ID"),
                Child, (IPTR)(producer_txt = TextObject,
                    TextFrame,
                End),
            End),
        End),
        TAG_MORE, (IPTR)msg->ops_AttrList
    );

    if (self)
    {
        struct DevicePage_DATA *data = INST_DATA(cl, self);

        data->name_txt = name_txt;
        data->hardwarename_txt = hardwarename_txt;
        data->product_txt = product_txt;
        data->producername_txt = producername_txt;
        data->producer_txt = producer_txt;
    }

    return self;
}


static IPTR DevicePage__MUIM_DevicePage_Update(Class *cl, Object *obj, struct MUIP_DevicePage_Update *msg)
{
    D(bug("MUIM_DevicePage_Update: device object %p\n", msg->device_obj));

    struct DevicePage_DATA *data = INST_DATA(cl, obj);

    OOP_Object *device_obj = msg->device_obj;
    STRPTR string;
    IPTR number;
    TEXT buffer[20];

    OOP_GetAttr(device_obj, aHidd_Name, (IPTR *)&string);
    SET(data->name_txt, MUIA_Text_Contents, string);

    OOP_GetAttr(device_obj, aHidd_HardwareName, (IPTR *)&string);
    SET(data->hardwarename_txt, MUIA_Text_Contents, string);

    OOP_GetAttr(device_obj, aHidd_Product, &number);
    sprintf(buffer, "%ld", number);
    SET(data->product_txt, MUIA_Text_Contents, buffer);

    OOP_GetAttr(device_obj, aHidd_ProducerName, (IPTR *)&string);
    SET(data->producername_txt, MUIA_Text_Contents, string);

    OOP_GetAttr(device_obj, aHidd_Producer, &number);
    sprintf(buffer, "%ld", number);
    SET(data->producer_txt, MUIA_Text_Contents, buffer);

    return 0;
}


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_2
(
    DevicePage, NULL, MUIC_Group, NULL,
    OM_NEW,                     struct opSet *,
    MUIM_DevicePage_Update,     struct MUIP_DevicePage_Update *
);
