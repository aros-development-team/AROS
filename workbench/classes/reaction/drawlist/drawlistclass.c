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
            case DRAWLIST_NumDirectives:
                data->dd_NumDirectives = tag->ti_Data;
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

        case DRAWLIST_NumDirectives:
            *msg->opg_Storage = data->dd_NumDirectives;
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
    WORD curX, curY;
    ULONG i;

    if (!rp || !data->dd_Directives || data->dd_NumDirectives == 0)
        return FALSE;

    baseX = im->LeftEdge + msg->imp_Offset.X;
    baseY = im->TopEdge + msg->imp_Offset.Y;
    curX = baseX;
    curY = baseY;

    SetDrMd(rp, JAM1);

    dl = data->dd_Directives;

    for (i = 0; i < data->dd_NumDirectives; i++, dl++)
    {
        switch (dl->dl_Command)
        {
            case DLD_END:
                return TRUE;

            case DLD_MOVE:
                curX = baseX + dl->dl_X;
                curY = baseY + dl->dl_Y;
                Move(rp, curX, curY);
                break;

            case DLD_DRAW:
                curX = baseX + dl->dl_X;
                curY = baseY + dl->dl_Y;
                Draw(rp, curX, curY);
                break;

            case DLD_FILL:
            {
                WORD fillX = baseX + dl->dl_X;
                WORD fillY = baseY + dl->dl_Y;
                RectFill(rp, curX, curY, fillX, fillY);
                break;
            }

            case DLD_NOPEN:
                SetAPen(rp, dl->dl_X);
                break;

            case DLD_FILLPEN:
                SetAPen(rp, dl->dl_X);
                break;

            case DLD_BPEN:
                SetBPen(rp, dl->dl_X);
                break;

            case DLD_AFPT:
                SetAfPt(rp, (UWORD *)(IPTR)dl->dl_X, dl->dl_Y);
                break;

            case DLD_AFPTSIZE:
                SetAfPt(rp, NULL, dl->dl_X);
                break;
        }
    }

    return TRUE;
}
