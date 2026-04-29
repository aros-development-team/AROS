/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction drawlist.image - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <graphics/gfxmacros.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <images/drawlist.h>
#include <utility/tagitem.h>

#include <string.h>

#include "drawlist_intern.h"

#define DrawListBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void drawlist_set(Class *cl, Object *o, struct opSet *msg)
{
    struct DrawListData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case DRAWLIST_Directives:
                data->dd_Directives = (struct DrawList *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR DrawList__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct DrawListData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct DrawListData));

        drawlist_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR DrawList__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR DrawList__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    drawlist_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR DrawList__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct DrawListData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case DRAWLIST_Directives:
            *msg->opg_Storage = (IPTR)data->dd_Directives;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR DrawList__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct DrawListData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawList *dl;
    WORD baseX, baseY;

    if (!rp || !data->dd_Directives)
        return FALSE;

    baseX = im->LeftEdge + msg->imp_Offset.X;
    baseY = im->TopEdge + msg->imp_Offset.Y;

    SetDrMd(rp, JAM1);

    for (dl = data->dd_Directives; dl->dl_Directive != DLST_END; dl++)
    {
        switch (dl->dl_Directive)
        {
            case DLST_LINE:
                SetAPen(rp, dl->dl_Pen);
                Move(rp, baseX + dl->dl_X1, baseY + dl->dl_Y1);
                Draw(rp, baseX + dl->dl_X2, baseY + dl->dl_Y2);
                break;

            case DLST_RECT:
            {
                WORD x1 = baseX + dl->dl_X1, y1 = baseY + dl->dl_Y1;
                WORD x2 = baseX + dl->dl_X2, y2 = baseY + dl->dl_Y2;
                SetAPen(rp, dl->dl_Pen);
                Move(rp, x1, y1);
                Draw(rp, x2, y1);
                Draw(rp, x2, y2);
                Draw(rp, x1, y2);
                Draw(rp, x1, y1);
                break;
            }

            case DLST_FILL:
                SetAPen(rp, dl->dl_Pen);
                RectFill(rp, baseX + dl->dl_X1, baseY + dl->dl_Y1,
                             baseX + dl->dl_X2, baseY + dl->dl_Y2);
                break;

            case DLST_AMOVE:
                Move(rp, baseX + dl->dl_X1, baseY + dl->dl_Y1);
                break;

            case DLST_ADRAW:
                SetAPen(rp, dl->dl_Pen);
                Draw(rp, baseX + dl->dl_X1, baseY + dl->dl_Y1);
                break;

            case DLST_AFILL:
                SetAPen(rp, dl->dl_Pen);
                RectFill(rp, baseX + dl->dl_X1, baseY + dl->dl_Y1,
                             baseX + dl->dl_X2, baseY + dl->dl_Y2);
                break;
        }
    }

    return TRUE;
}
