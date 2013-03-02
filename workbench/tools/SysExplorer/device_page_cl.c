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

#define DEBUG 1
#include <aros/debug.h>

#include <zune/customclasses.h>


/*** Instance Data **********************************************************/
struct DevicePage_DATA
{
    /* Nothing to add here */
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
            MUIA_FrameTitle, (IPTR)"General",
            GroupFrame,
            Child, (IPTR)(ColGroup(2),
                Child, (IPTR)Label("Name"),
                Child, (IPTR)(name_txt = TextObject,
                    TextFrame,
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
        TAG_DONE
    );

    if (self)
    {
        OOP_Object *device_obj = (OOP_Object *)GetTagData(MUIA_PropertyWin_Object, 0, msg->ops_AttrList);
        STRPTR string;
        IPTR number;
        TEXT buffer[20];

        OOP_GetAttr(device_obj, aHidd_Name, (IPTR *)&string);
        SET(name_txt, MUIA_Text_Contents, string);

        OOP_GetAttr(device_obj, aHidd_HardwareName, (IPTR *)&string);
        SET(hardwarename_txt, MUIA_Text_Contents, string);

        OOP_GetAttr(device_obj, aHidd_Product, &number);
        sprintf(buffer, "%ld", number);
        SET(product_txt, MUIA_Text_Contents, buffer);

        OOP_GetAttr(device_obj, aHidd_ProducerName, (IPTR *)&string);
        SET(producername_txt, MUIA_Text_Contents, string);

        OOP_GetAttr(device_obj, aHidd_Producer, &number);
        sprintf(buffer, "%ld", number);
        SET(producer_txt, MUIA_Text_Contents, buffer);
    }

    return self;
}


/*** Setup ******************************************************************/
ZUNE_CUSTOMCLASS_1
(
    DevicePage, NULL, MUIC_Group, NULL,
    OM_NEW, struct opSet *
);
