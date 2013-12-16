/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/intuition.h>
#include <proto/alib.h>

#include <zune/customclasses.h>

#include "languagelist.h"

struct Languagelist_DATA
{
};

struct MUI_CustomClass     *Languagelist_CLASS;


IPTR Languagelist__MUIM_DragQuery(struct IClass *cl, Object *obj,
    struct MUIP_DragQuery *msg)
{
    if ((IPTR)msg->obj == XGET(obj, MUIA_UserData))
        return MUIV_DragQuery_Accept;
    else
        return MUIV_DragQuery_Refuse;
}

IPTR Languagelist__MUIM_DragDrop(struct IClass *cl, Object *obj,
    struct MUIP_DragDrop *msg)
{
    struct MUI_List_TestPos_Result pos;
    LONG n;
    CONST_STRPTR str;

    /* Find drop position */

    DoMethod(obj, MUIM_List_TestPos, msg->x, msg->y, (IPTR) &pos);
    if (pos.entry != -1)
    {
        /* Change drop position when coords move past centre of entry, not
         * entry boundary */

        n = pos.entry;
        if (pos.yoffset > 0)
            n++;
    }
    else if ((pos.flags & MUI_LPR_ABOVE) != 0)
        n = MUIV_List_Insert_Top;
    else
        n = MUIV_List_Insert_Bottom;

    DoMethod(msg->obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &str);
    DoMethod(msg->obj, MUIM_List_Remove, MUIV_List_Remove_Active);
    DoMethod(obj, MUIM_List_InsertSingle, str, n);

    return TRUE;
}

ZUNE_CUSTOMCLASS_2
(
    Languagelist, NULL, MUIC_List, NULL,
    MUIM_DragQuery,  struct MUIP_DragQuery *,
    MUIM_DragDrop,  struct MUIP_DragDrop *
);
