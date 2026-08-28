/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction drawlist.image - BOOPSI class implementation
*/
#define DEBUG 1

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
            case DRAWLIST_RefWidth:
                data->dd_RefWidth = (WORD)tag->ti_Data;
                break;
            case DRAWLIST_RefHeight:
                data->dd_RefHeight = (WORD)tag->ti_Data;
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
        D(bug("[DrawList] OM_NEW: obj 0x%p\n", (APTR)retval));
        struct DrawListData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct DrawListData));

        drawlist_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR DrawList__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[DrawList] OM_DISPOSE: obj 0x%p\n", o));
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR DrawList__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[DrawList] OM_SET: obj 0x%p\n", o));
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
        case DRAWLIST_RefWidth:
            *msg->opg_Storage = data->dd_RefWidth;
            return TRUE;
        case DRAWLIST_RefHeight:
            *msg->opg_Storage = data->dd_RefHeight;
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
    struct AreaInfo ai, *old_ai;
    struct TmpRas tr, *old_tr;
    PLANEPTR raster;
    UWORD areabuf[5 * 64 / 2];
    WORD baseX, baseY, w, h, refW, refH;
    BOOL inarea = FALSE;

    D(bug("[DrawList] IM_DRAW: state %d, dimensions %dx%d\n", msg->imp_State, im->Width, im->Height));

    if (!rp || !data->dd_Directives)
        return FALSE;

    baseX = im->LeftEdge + msg->imp_Offset.X;
    baseY = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    /* Directive coordinates are relative to the reference size and are
     * scaled to the actual image size */
    refW = data->dd_RefWidth;
    refH = data->dd_RefHeight;

#define SX(v) (baseX + (refW ? ((v) * (w - 1)) / refW : (v)))
#define SY(v) (baseY + (refH ? ((v) * (h - 1)) / refH : (v)))
#define SW(v) (refW ? ((v) * (w - 1)) / refW : (v))
#define SH(v) (refH ? ((v) * (h - 1)) / refH : (v))

    /* The DLST_AMOVE/DLST_ADRAW/DLST_AFILL directives are area (polygon)
     * fill operations, so give the rastport temporary area workspace */
    old_ai = rp->AreaInfo;
    old_tr = rp->TmpRas;

    raster = AllocRaster(w + 16, h + 16);
    if (raster)
    {
        InitArea(&ai, areabuf, sizeof(areabuf) / 5);
        InitTmpRas(&tr, raster, RASSIZE(w + 16, h + 16));
        rp->AreaInfo = &ai;
        rp->TmpRas = &tr;
    }

    SetDrMd(rp, JAM1);

    for (dl = data->dd_Directives; dl->dl_Directive != DLST_END; dl++)
    {
        switch (dl->dl_Directive)
        {
            case DLST_LINE:
                SetAPen(rp, dl->dl_Pen);
                Move(rp, SX(dl->dl_X1), SY(dl->dl_Y1));
                Draw(rp, SX(dl->dl_X2), SY(dl->dl_Y2));
                break;

            case DLST_RECT:
            {
                WORD x1 = SX(dl->dl_X1), y1 = SY(dl->dl_Y1);
                WORD x2 = SX(dl->dl_X2), y2 = SY(dl->dl_Y2);
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
                RectFill(rp, SX(dl->dl_X1), SY(dl->dl_Y1),
                             SX(dl->dl_X2), SY(dl->dl_Y2));
                break;

            case DLST_ELLIPSE:
                SetAPen(rp, dl->dl_Pen);
                DrawEllipse(rp, SX(dl->dl_X1), SY(dl->dl_Y1),
                                SW(dl->dl_X2), SH(dl->dl_Y2));
                break;

            case DLST_CIRCLE:
                SetAPen(rp, dl->dl_Pen);
                DrawEllipse(rp, SX(dl->dl_X1), SY(dl->dl_Y1),
                                SW(dl->dl_X2), SH(dl->dl_X2));
                break;

            case DLST_AMOVE:
                if (raster)
                {
                    AreaMove(rp, SX(dl->dl_X1), SY(dl->dl_Y1));
                    inarea = TRUE;
                }
                else
                    Move(rp, SX(dl->dl_X1), SY(dl->dl_Y1));
                break;

            case DLST_ADRAW:
                if (raster)
                    AreaDraw(rp, SX(dl->dl_X1), SY(dl->dl_Y1));
                else
                {
                    SetAPen(rp, dl->dl_Pen);
                    Draw(rp, SX(dl->dl_X1), SY(dl->dl_Y1));
                }
                break;

            case DLST_AFILL:
                if (inarea)
                {
                    SetAPen(rp, dl->dl_Pen);
                    SetOPen(rp, dl->dl_Pen);
                    AreaEnd(rp);
                    BNDRYOFF(rp);
                    inarea = FALSE;
                }
                break;

            case DLST_LINESIZE:
                /* Line thickness is not supported by graphics.library */
                break;
        }
    }

    if (raster)
    {
        rp->AreaInfo = old_ai;
        rp->TmpRas = old_tr;
        FreeRaster(raster, w + 16, h + 16);
    }

#undef SX
#undef SY
#undef SW
#undef SH

    return TRUE;
}
