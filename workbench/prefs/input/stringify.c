/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <stdio.h>

#include "stringify.h"

struct MUI_CustomClass *StringifyClass;

struct Stringify_DATA
{
    UWORD   Type;
    char    buf[16];
};


static IPTR Stringify__OM_NEW(Class  * cl, Object * obj, struct opSet * msg)
{
    obj = (Object*) DoSuperMethodA(cl, obj, (Msg)msg);
    if (obj != NULL)
    {
        struct Stringify_DATA *data = INST_DATA(cl,obj);
        data->Type = (UWORD) GetTagData(MUIA_MyStringifyType, 0, msg->ops_AttrList);
    }
    return (IPTR) obj;
}


static IPTR Stringify__MUIM_Numeric_Stringify(Class  * cl, Object * obj, Msg msg)
{
    struct Stringify_DATA *data = INST_DATA(cl,obj);

    struct MUIP_Numeric_Stringify *m = (APTR)msg;

    if (data->Type == STRINGIFY_RepeatRate)
    {
        sprintf((char *)data->buf,"%3.2fs", 0.02 * (12 - m->value));
    }
    else if (data->Type == STRINGIFY_RepeatDelay)
    {
        sprintf((char *)data->buf,"%dms", 20 + 20 * m->value);
    }
    else if (data->Type == STRINGIFY_DoubleClickDelay)
    {
        sprintf((char *)data->buf,"%3.2fs", 0.02 + 0.02 * m->value);
    }
    return (IPTR) data->buf;
}


ZUNE_CUSTOMCLASS_2
(
    Stringify, NULL, MUIC_Slider, NULL,
    OM_NEW,                  struct opSet *,
    MUIM_Numeric_Stringify,  Msg
);
