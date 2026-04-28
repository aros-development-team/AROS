/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction glyph.image - BOOPSI class implementation
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
#include <images/glyph.h>
#include <utility/tagitem.h>

#include <string.h>

#include "glyph_intern.h"

#define GlyphBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void glyph_set(Class *cl, Object *o, struct opSet *msg)
{
    struct GlyphData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case GLYPH_Glyph:
                data->gd_Glyph = tag->ti_Data;
                break;
            case GLYPH_SoftStyle:
                data->gd_SoftStyle = tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void glyph_draw_arrow_up(struct RastPort *rp, WORD x, WORD y,
    WORD w, WORD h)
{
    WORD cx = x + w / 2;
    WORD row;

    for (row = 0; row < h; row++)
    {
        WORD span = (row * w) / (2 * h);
        Move(rp, cx - span, y + row);
        Draw(rp, cx + span, y + row);
    }
}

static void glyph_draw_arrow_down(struct RastPort *rp, WORD x, WORD y,
    WORD w, WORD h)
{
    WORD cx = x + w / 2;
    WORD row;

    for (row = 0; row < h; row++)
    {
        WORD span = ((h - 1 - row) * w) / (2 * h);
        Move(rp, cx - span, y + row);
        Draw(rp, cx + span, y + row);
    }
}

static void glyph_draw_arrow_left(struct RastPort *rp, WORD x, WORD y,
    WORD w, WORD h)
{
    WORD cy = y + h / 2;
    WORD col;

    for (col = 0; col < w; col++)
    {
        WORD span = (col * h) / (2 * w);
        Move(rp, x + col, cy - span);
        Draw(rp, x + col, cy + span);
    }
}

static void glyph_draw_arrow_right(struct RastPort *rp, WORD x, WORD y,
    WORD w, WORD h)
{
    WORD cy = y + h / 2;
    WORD col;

    for (col = 0; col < w; col++)
    {
        WORD span = ((w - 1 - col) * h) / (2 * w);
        Move(rp, x + col, cy - span);
        Draw(rp, x + col, cy + span);
    }
}

static void glyph_draw_popup(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    WORD bx = x + 2;
    WORD by = y + 2;
    WORD bw = w - 4;
    WORD bh = h - 4;

    if (bw < 3 || bh < 3)
    {
        bx = x;
        by = y;
        bw = w;
        bh = h;
    }

    /* Draw a small rectangle representing a popup */
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, bx, by + bh - 1);
    Draw(rp, bx, by);
    Draw(rp, bx + bw - 1, by);

    SetAPen(rp, pens[SHADOWPEN]);
    Draw(rp, bx + bw - 1, by + bh - 1);
    Draw(rp, bx, by + bh - 1);

    /* Down arrow indicator at bottom */
    if (bh > 5)
    {
        WORD ax = bx + bw / 2;
        WORD ay = by + bh - 3;

        SetAPen(rp, pens[TEXTPEN]);
        Move(rp, ax - 2, ay);
        Draw(rp, ax + 2, ay);
        Move(rp, ax - 1, ay + 1);
        Draw(rp, ax + 1, ay + 1);
        WritePixel(rp, ax, ay + 2);
    }
}

static void glyph_draw_checkbox(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Draw checkbox border */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHINEPEN]);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);

    /* Fill interior */
    if (w > 4 && h > 4)
    {
        SetAPen(rp, pens[BACKGROUNDPEN]);
        RectFill(rp, x + 2, y + 2, x + w - 3, y + h - 3);
    }
}

static void glyph_draw_radiobutton(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    WORD cx = x + w / 2;
    WORD cy = y + h / 2;
    WORD r = (w < h ? w : h) / 2 - 1;

    if (r < 2)
        r = 2;

    /* Draw circular border using arcs approximated by lines */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, cx - r, cy);
    Draw(rp, cx, cy - r);
    Draw(rp, cx + r, cy);

    SetAPen(rp, pens[SHINEPEN]);
    Draw(rp, cx, cy + r);
    Draw(rp, cx - r, cy);
}

static void glyph_draw_popfile(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    WORD bx = x + 1;
    WORD by = y + 1;
    WORD bw = w - 2;
    WORD bh = h - 2;
    WORD fold = bw > 6 ? 3 : bw / 3;

    if (bw < 3 || bh < 3)
        return;

    /* Draw file icon with folded corner */
    SetAPen(rp, pens[TEXTPEN]);
    Move(rp, bx, by);
    Draw(rp, bx + bw - fold - 1, by);
    Draw(rp, bx + bw - 1, by + fold);
    Draw(rp, bx + bw - 1, by + bh - 1);
    Draw(rp, bx, by + bh - 1);
    Draw(rp, bx, by);

    /* Fold line */
    Move(rp, bx + bw - fold - 1, by);
    Draw(rp, bx + bw - fold - 1, by + fold);
    Draw(rp, bx + bw - 1, by + fold);
}

static void glyph_draw_popdrawer(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    WORD bx = x + 1;
    WORD by = y + 1;
    WORD bw = w - 2;
    WORD bh = h - 2;
    WORD tab = bw > 8 ? bw / 3 : bw / 2;

    if (bw < 3 || bh < 3)
        return;

    /* Draw folder icon */
    SetAPen(rp, pens[TEXTPEN]);

    /* Tab */
    Move(rp, bx, by + 2);
    Draw(rp, bx, by);
    Draw(rp, bx + tab, by);
    Draw(rp, bx + tab + 1, by + 1);
    Draw(rp, bx + tab + 1, by + 2);

    /* Body */
    Draw(rp, bx + bw - 1, by + 2);
    Draw(rp, bx + bw - 1, by + bh - 1);
    Draw(rp, bx, by + bh - 1);
    Draw(rp, bx, by + 2);
}

/******************************************************************************/

IPTR Glyph__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct GlyphData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct GlyphData));
        data->gd_Glyph = GLYPH_POPUP;

        glyph_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Glyph__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Glyph__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    glyph_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Glyph__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct GlyphData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case GLYPH_Glyph:
            *msg->opg_Storage = data->gd_Glyph;
            return TRUE;

        case GLYPH_SoftStyle:
            *msg->opg_Storage = data->gd_SoftStyle;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Glyph__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct GlyphData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    UWORD *pens;
    WORD x, y, w, h;
    UWORD fgPen, bgPen;

    if (!rp || !dri)
        return FALSE;

    pens = dri->dri_Pens;
    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    if (w < 2 || h < 2)
        return TRUE;

    /* Select pens based on state */
    switch (msg->imp_State)
    {
        case IDS_SELECTED:
        case IDS_INACTIVESELECTED:
            fgPen = pens[FILLTEXTPEN];
            bgPen = pens[FILLPEN];
            break;

        default:
            fgPen = pens[TEXTPEN];
            bgPen = pens[BACKGROUNDPEN];
            break;
    }

    /* Clear background */
    SetAPen(rp, bgPen);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    SetDrMd(rp, JAM1);

    switch (data->gd_Glyph)
    {
        case GLYPH_ARROWUP:
        case GLYPH_UPARROW:
            SetAPen(rp, fgPen);
            glyph_draw_arrow_up(rp, x + 2, y + 2, w - 4, h - 4);
            break;

        case GLYPH_ARROWDOWN:
        case GLYPH_DOWNARROW:
            SetAPen(rp, fgPen);
            glyph_draw_arrow_down(rp, x + 2, y + 2, w - 4, h - 4);
            break;

        case GLYPH_ARROWLEFT:
        case GLYPH_LEFTARROW:
            SetAPen(rp, fgPen);
            glyph_draw_arrow_left(rp, x + 2, y + 2, w - 4, h - 4);
            break;

        case GLYPH_ARROWRIGHT:
        case GLYPH_RIGHTARROW:
            SetAPen(rp, fgPen);
            glyph_draw_arrow_right(rp, x + 2, y + 2, w - 4, h - 4);
            break;

        case GLYPH_POPUP:
            glyph_draw_popup(rp, pens, x, y, w, h);
            break;

        case GLYPH_POPFILE:
            glyph_draw_popfile(rp, pens, x, y, w, h);
            break;

        case GLYPH_POPDRAWER:
            glyph_draw_popdrawer(rp, pens, x, y, w, h);
            break;

        case GLYPH_POPFONT:
        case GLYPH_POPSCREEN:
        case GLYPH_POPTIME:
            /* These popup variants use the generic popup glyph */
            glyph_draw_popup(rp, pens, x, y, w, h);
            break;

        case GLYPH_CHECKBOX:
            glyph_draw_checkbox(rp, pens, x, y, w, h);
            break;

        case GLYPH_RADIOBUTTON:
            glyph_draw_radiobutton(rp, pens, x, y, w, h);
            break;

        default:
            glyph_draw_popup(rp, pens, x, y, w, h);
            break;
    }

    /* Draw disabled ghosting pattern */
    if (msg->imp_State == IDS_DISABLED || msg->imp_State == IDS_INACTIVEDISABLED)
    {
        UWORD ghostPat[] = { 0x2222, 0x8888 };

        SetAPen(rp, pens[BLOCKPEN]);
        SetAfPt(rp, ghostPat, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    return TRUE;
}
