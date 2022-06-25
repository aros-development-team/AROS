/*
    Copyright (C) 2022, The AROS Development Team. All rights reserved.
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <stdio.h>

#include "aslstringify.h"

struct AslStringify_DATA
{
    UWORD   Type;
    char    buf[5];
};


static IPTR AslStringify__OM_NEW(Class  * cl, Object * obj, struct opSet * msg)
{
    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    obj = (Object*) DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj != NULL)
    {
#if (0)
        struct AslStringify_DATA *data = INST_DATA(cl,obj);
#endif
    }
    return (IPTR) obj;
}


static IPTR AslStringify__MUIM_Numeric_Stringify(Class  * cl, Object * obj, struct MUIP_Numeric_Stringify *msg)
{
    struct AslStringify_DATA *data = INST_DATA(cl,obj);

    D(bug("[AslEditor.class] %s()\n", __PRETTY_FUNCTION__));

    sprintf((char *)data->buf, "%3d%%", (int)msg->value);

    return (IPTR)data->buf;
}


ZUNE_CUSTOMCLASS_2
(
    AslStringify, NULL, MUIC_Slider, NULL,
    OM_NEW,                  struct opSet *,
    MUIM_Numeric_Stringify,  struct MUIP_Numeric_Stringify *
);
