/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#define DEBUG 1
#include <aros/debug.h>

#include <proto/alib.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/muimaster.h>

#include <libraries/mui.h>
#include <zune/customclasses.h>

#include <stdio.h>

#include "reqtoolsstringify.h"

struct ReqToolsStringify_DATA
{
    UWORD   Type;
    char    buf[5];
};


static IPTR ReqToolsStringify__OM_NEW(Class  * cl, Object * obj, struct opSet * msg)
{
    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    obj = (Object*) DoSuperMethodA(cl, obj, (Msg)msg);

    if (obj != NULL)
    {
#if (0)
        struct ReqToolsStringify_DATA *data = INST_DATA(cl,obj);
#endif
    }
    return (IPTR) obj;
}


static IPTR ReqToolsStringify__MUIM_Numeric_Stringify(Class  * cl, Object * obj, struct MUIP_Numeric_Stringify *msg)
{
    struct ReqToolsStringify_DATA *data = INST_DATA(cl,obj);

    D(bug("[ReqToolsEditor.class] %s()\n", __PRETTY_FUNCTION__));

    sprintf((char *)data->buf, "%3d%%", msg->value);

    return (IPTR)data->buf;
}


ZUNE_CUSTOMCLASS_2
(
    ReqToolsStringify, NULL, MUIC_Slider, NULL,
    OM_NEW,                  struct opSet *,
    MUIM_Numeric_Stringify,  struct MUIP_Numeric_Stringify *
);
