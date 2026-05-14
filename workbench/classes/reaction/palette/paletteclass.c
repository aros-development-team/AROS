/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction palette.gadget - BOOPSI class implementation
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
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/palette.h>
#include <images/bevel.h>
#include <utility/tagitem.h>

#include <string.h>

#include "palette_intern.h"

#define PaletteBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void palette_set(Class *cl, Object *o, struct opSet *msg)
{
    struct PaletteGadData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case PALETTE_Color:
                data->pd_Color = tag->ti_Data;
                break;
            case PALETTE_ColorOffset:
                data->pd_ColorOffset = tag->ti_Data;
                break;
            case PALETTE_NumColors:
                data->pd_NumColors = tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR Palette__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Palette] OM_NEW: entry\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        D(bug("[Palette] OM_NEW: obj=%p\n", (Object *)retval));
        struct PaletteGadData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct PaletteGadData));

        /* Set defaults */
        data->pd_NumColors    = 8;
        data->pd_ColorOffset  = 0;
        data->pd_Color        = 0;

        palette_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Palette__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Palette] OM_DISPOSE: entry\n"));
    struct PaletteGadData *data = INST_DATA(cl, o);
    if (data->pd_FrameBevel) { DisposeObject(data->pd_FrameBevel); data->pd_FrameBevel = NULL; }
    if (data->pd_SelBevel)   { DisposeObject(data->pd_SelBevel);   data->pd_SelBevel   = NULL; }
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Palette__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Palette] OM_SET: entry\n"));
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    palette_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Palette__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PaletteGadData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case PALETTE_Color:
            *msg->opg_Storage = data->pd_Color;
            return TRUE;

        case PALETTE_ColorOffset:
            *msg->opg_Storage = data->pd_ColorOffset;
            return TRUE;

        case PALETTE_NumColors:
            *msg->opg_Storage = data->pd_NumColors;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Palette__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    D(bug("[Palette] GM_RENDER: redraw=%d\n", msg->gpr_Redraw));
    struct PaletteGadData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    ULONG cols, rows;
    WORD cellw, cellh;
    ULONG i;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    cols = data->pd_NumColors;
    if (cols == 0) cols = 1;

    rows = (data->pd_NumColors + cols - 1) / cols;
    if (rows == 0) rows = 1;

    cellw = w / cols;
    cellh = h / rows;
    if (cellw < 1) cellw = 1;
    if (cellh < 1) cellh = 1;

    /* Clear gadget area */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Draw color cells */
    for (i = 0; i < data->pd_NumColors; i++)
    {
        WORD col = i % cols;
        WORD row = i / cols;
        WORD cx = x + col * cellw;
        WORD cy = y + row * cellh;

        SetAPen(rp, data->pd_ColorOffset + i);
        RectFill(rp, cx + 1, cy + 1, cx + cellw - 2, cy + cellh - 2);

        /* Highlight selected color with a frame */
    /* Highlight selected color via bevel.image (BVS_BUTTON raised). */
        if (i == data->pd_Color)
        {
            if (dri)
            {
                if (!data->pd_SelBevel)
                {
                    data->pd_SelBevel = NewObject(NULL, "bevel.image",
                        BEVEL_Style, BVS_BUTTON,
                        BEVEL_Transparent, TRUE,
                        TAG_END);
                }
                if (data->pd_SelBevel)
                {
                    struct impDraw idmsg;
                    idmsg.MethodID         = IM_DRAWFRAME;
                    idmsg.imp_RPort        = rp;
                    idmsg.imp_Offset.X     = cx;
                    idmsg.imp_Offset.Y     = cy;
                    idmsg.imp_State        = IDS_SELECTED;
                    idmsg.imp_DrInfo       = dri;
                    idmsg.imp_Dimensions.Width  = cellw;
                    idmsg.imp_Dimensions.Height = cellh;
                    DoMethodA(data->pd_SelBevel, (Msg)&idmsg);
                }
            }
        }
    }

    /* Disabled rendering */
    if (gad->Flags & GFLG_DISABLED)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}
