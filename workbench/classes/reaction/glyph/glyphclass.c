/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction glyph.image - BOOPSI class implementation
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
#include <images/glyph.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

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
        D(bug("[Glyph] OM_NEW: obj 0x%p\n", (APTR)retval));
        struct GlyphData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct GlyphData));
        data->gd_Glyph = GLYPH_POPUP;

        /* Snapshot the user's preferred glyph rendering style */
        {
            struct UIPrefs *prefs;
            prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
            if (prefs)
            {
                ObtainSemaphoreShared(&prefs->cap_Semaphore);
                data->gd_VisualType = prefs->cap_GlyphType;
                ReleaseSemaphore(&prefs->cap_Semaphore);
            }
        }

        glyph_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Glyph__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Glyph] OM_DISPOSE: obj 0x%p\n", o));
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Glyph__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Glyph] OM_SET: obj 0x%p\n", o));
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

    D(bug("[Glyph] IM_DRAW: state %d, dimensions %dx%d, glyph %d\n", msg->imp_State, im->Width, im->Height, data->gd_Glyph));

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

    /* Helper to draw the glyph at (x,y) using fgPen for the strokes. We
     * encapsulate it so GLT_3D can call it twice (shadow pre-pass plus the
     * main pass shifted up-left). */
    #define DRAW_GLYPH(_x,_y,_pen) do { \
        WORD _gx = (_x), _gy = (_y); \
        switch (data->gd_Glyph) { \
            case GLYPH_UPARROW: case GLYPH_BUPARROW: \
                SetAPen(rp, (_pen)); glyph_draw_arrow_up   (rp, _gx + 2, _gy + 2, w - 4, h - 4); break; \
            case GLYPH_DOWNARROW: case GLYPH_BDOWNARROW: \
            case GLYPH_DROPDOWN: case GLYPH_DROPDOWNMENU: \
                SetAPen(rp, (_pen)); glyph_draw_arrow_down (rp, _gx + 2, _gy + 2, w - 4, h - 4); break; \
            case GLYPH_LEFTARROW: case GLYPH_BLEFTARROW: \
                SetAPen(rp, (_pen)); glyph_draw_arrow_left (rp, _gx + 2, _gy + 2, w - 4, h - 4); break; \
            case GLYPH_RIGHTARROW: case GLYPH_BRIGHTARROW: \
            case GLYPH_RETURNARROW: \
                SetAPen(rp, (_pen)); glyph_draw_arrow_right(rp, _gx + 2, _gy + 2, w - 4, h - 4); break; \
            case GLYPH_POPUP: case GLYPH_POPFONT: case GLYPH_POPSCREENMODE: \
            case GLYPH_POPTIME: case GLYPH_CYCLE: \
                glyph_draw_popup    (rp, pens, _gx, _gy, w, h); break; \
            case GLYPH_POPFILE: \
                glyph_draw_popfile  (rp, pens, _gx, _gy, w, h); break; \
            case GLYPH_POPDRAWER: \
                glyph_draw_popdrawer(rp, pens, _gx, _gy, w, h); break; \
            case GLYPH_CHECKMARK: \
                glyph_draw_checkbox (rp, pens, _gx, _gy, w, h); break; \
            case GLYPH_RADIOBUTTON: \
                glyph_draw_radiobutton(rp, pens, _gx, _gy, w, h); break; \
            default: break; \
        } } while (0)

    if (data->gd_VisualType == GLT_3D &&
        msg->imp_State != IDS_DISABLED && msg->imp_State != IDS_INACTIVEDISABLED)
    {
        /* Shadow pre-pass shifted +1,+1 then redraw on top in fgPen. Only
         * for arrow glyphs - composite popup/checkbox glyphs already render
         * their own shading internally. */
        switch (data->gd_Glyph)
        {
            case GLYPH_UPARROW: case GLYPH_BUPARROW:
            case GLYPH_DOWNARROW: case GLYPH_BDOWNARROW:
            case GLYPH_LEFTARROW: case GLYPH_BLEFTARROW:
            case GLYPH_RIGHTARROW: case GLYPH_BRIGHTARROW:
            case GLYPH_DROPDOWN: case GLYPH_DROPDOWNMENU:
            case GLYPH_RETURNARROW:
                DRAW_GLYPH(x + 1, y + 1, pens[SHADOWPEN]);
                DRAW_GLYPH(x,     y,     pens[SHINEPEN]);
                break;
            default:
                DRAW_GLYPH(x, y, fgPen);
                break;
        }
    }
    else
    {
        DRAW_GLYPH(x, y, fgPen);
    }

    #undef DRAW_GLYPH

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
