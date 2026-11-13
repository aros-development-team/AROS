/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: field frame of string.gadget and integer.gadget: a BVS_FIELD bevel
          around the text area. The gadget box is the text area, which
          Intuition's string code edits in; the frame box is kept here.
*/

#ifndef REACTION_FIELD_H
#define REACTION_FIELD_H

#include <intuition/intuition.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <images/bevel.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/alib.h>

#include "reaction_box.h"

#define FIELD_DX 4
#define FIELD_DY 2

struct ReactionField
{
    Object *rf_Bevel;
    WORD    rf_Left, rf_Top, rf_Width, rf_Height;  /* frame box, as GA_ gave it */
};

/* After the superclass applied the tags: take the frame box from what they
   set and keep the gadget box at the text area inside it */
static inline void field_set_box(struct ReactionField *f, struct Gadget *g,
                                 struct TagItem *tags, BOOL all)
{
    if (all || FindTagItem(GA_Left, tags) || FindTagItem(GA_RelRight, tags))
    {
        f->rf_Left = g->LeftEdge;
        g->LeftEdge = f->rf_Left + FIELD_DX;
    }
    if (all || FindTagItem(GA_Top, tags) || FindTagItem(GA_RelBottom, tags))
    {
        f->rf_Top = g->TopEdge;
        g->TopEdge = f->rf_Top + FIELD_DY;
    }
    if (all || FindTagItem(GA_Width, tags) || FindTagItem(GA_RelWidth, tags))
    {
        f->rf_Width = g->Width;
        g->Width = f->rf_Width - 2 * FIELD_DX;
    }
    if (all || FindTagItem(GA_Height, tags) || FindTagItem(GA_RelHeight, tags))
    {
        f->rf_Height = g->Height;
        g->Height = f->rf_Height - 2 * FIELD_DY;
    }
}

static inline BOOL field_get(struct ReactionField *f, struct opGet *msg)
{
    switch (msg->opg_AttrID)
    {
        case GA_Left:   *msg->opg_Storage = (IPTR)(LONG)f->rf_Left;   return TRUE;
        case GA_Top:    *msg->opg_Storage = (IPTR)(LONG)f->rf_Top;    return TRUE;
        case GA_Width:  *msg->opg_Storage = (IPTR)(LONG)f->rf_Width;  return TRUE;
        case GA_Height: *msg->opg_Storage = (IPTR)(LONG)f->rf_Height; return TRUE;
    }
    return FALSE;
}

/* The frame, on a full redraw; the bevel fills its inside, so it goes
   before the superclass draws the text */
static inline void field_draw_frame(struct ReactionField *f, struct Gadget *g,
                                    struct gpRender *msg)
{
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget box = *g;
    struct impDraw id;
    WORD x, y, w, h;

    if (msg->gpr_Redraw != GREDRAW_REDRAW || !rp || !dri)
        return;
    if (!f->rf_Bevel)
        f->rf_Bevel = NewObject(NULL, "bevel.image", BEVEL_Style, BVS_FIELD, TAG_END);
    if (!f->rf_Bevel)
        return;

    box.LeftEdge = f->rf_Left;
    box.TopEdge = f->rf_Top;
    box.Width = f->rf_Width;
    box.Height = f->rf_Height;
    reaction_gadget_box(&box, msg->gpr_GInfo, &x, &y, &w, &h);

    /* the strip between the frame and the text area */
    SetAPen(rp, dri->dri_Pens[BACKGROUNDPEN]);
    RectFill(rp, x, y, x + w - 1, y + FIELD_DY - 1);
    RectFill(rp, x, y + h - FIELD_DY, x + w - 1, y + h - 1);
    RectFill(rp, x, y, x + FIELD_DX - 1, y + h - 1);
    RectFill(rp, x + w - FIELD_DX, y, x + w - 1, y + h - 1);

    id.MethodID = IM_DRAWFRAME;
    id.imp_RPort = rp;
    id.imp_Offset.X = x;
    id.imp_Offset.Y = y;
    id.imp_State = (g->Flags & GFLG_DISABLED) ? IDS_DISABLED : IDS_NORMAL;
    id.imp_DrInfo = dri;
    id.imp_Dimensions.Width = w;
    id.imp_Dimensions.Height = h;
    DoMethodA(f->rf_Bevel, (Msg)&id);
}

static inline void field_dispose(struct ReactionField *f)
{
    if (f->rf_Bevel)
        DisposeObject(f->rf_Bevel);
    f->rf_Bevel = NULL;
}

/* minimum visible characters plus the frame */
static inline void field_domain(struct gpDomain *msg, UWORD chars)
{
    struct TextFont *font = NULL;
    UWORD cw = 8, ch = 8;

    if (msg->gpd_GInfo && msg->gpd_GInfo->gi_DrInfo)
        font = msg->gpd_GInfo->gi_DrInfo->dri_Font;
    if (!font && msg->gpd_RPort)
        font = msg->gpd_RPort->Font;
    if (font)
    {
        cw = font->tf_XSize;
        ch = font->tf_YSize;
    }
    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = (chars ? chars : 4) * cw + 2 * FIELD_DX + 4;
    msg->gpd_Domain.Height = ch + 2 * FIELD_DY + 4;
}

#endif /* REACTION_FIELD_H */
