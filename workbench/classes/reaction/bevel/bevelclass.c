/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bevel.image - BOOPSI class implementation
*/
#define DEBUG 1

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <images/bevel.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

#include <string.h>

#include "bevel_intern.h"

#define BevelBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void bevel_set(Class *cl, Object *o, struct opSet *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case BEVEL_Style:
                data->bd_Style = tag->ti_Data;
                break;
            case BEVEL_Label:
                data->bd_Label = (STRPTR)tag->ti_Data;
                break;
            case BEVEL_LabelPlace:
                data->bd_LabelPlace = tag->ti_Data;
                break;
            case BEVEL_TextPen:
                data->bd_TextPen = (UWORD)tag->ti_Data;
                break;
            case BEVEL_FillPen:
                data->bd_FillPen = (UWORD)tag->ti_Data;
                data->bd_FillPenSet = TRUE;
                break;
            case BEVEL_Transparent:
                data->bd_Transparent = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

/* Return the per-edge thickness (in pixels) for the given bevel style,
 * modulated by the user's cap_BevelType prefs. */
static UWORD bevel_thickness_for(ULONG style, UBYTE visual)
{
    if (style == BVS_NONE)
        return 0;
    /* Thin prefs force 1px borders regardless of style class */
    if (visual == BVT_THIN || visual == BVT_XENTHIN)
        return 1;
    switch (style)
    {
        case BVS_THIN:
        case BVS_BOX:
        case BVS_FOCUS:
        case BVS_SBAR_HORIZ:
        case BVS_SBAR_VERT:
            return 1;
        case BVS_BUTTON:
        case BVS_GROUP:
        case BVS_FIELD:
        case BVS_DROPBOX:
        case BVS_RADIOBUTTON:
        case BVS_STANDARD:
        default:
            return 2;
    }
}

/* (bevel_thickness wrapper removed - all callers now pass the visual type) */

/******************************************************************************/

static void bevel_draw_thin(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHADOWPEN]);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);
}

static void bevel_draw_button(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h, ULONG state)
{
    UWORD shinePen, shadowPen;

    if (state == IDS_SELECTED || state == IDS_INACTIVESELECTED)
    {
        shinePen = pens[SHADOWPEN];
        shadowPen = pens[SHINEPEN];
    }
    else
    {
        shinePen = pens[SHINEPEN];
        shadowPen = pens[SHADOWPEN];
    }

    /* Outer top-left highlight */
    SetAPen(rp, shinePen);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    /* Outer bottom-right shadow */
    SetAPen(rp, shadowPen);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);

    /* Inner top-left highlight */
    SetAPen(rp, shinePen);
    Move(rp, x + 1, y + h - 2);
    Draw(rp, x + 1, y + 1);
    Draw(rp, x + w - 2, y + 1);

    /* Inner bottom-right shadow */
    SetAPen(rp, shadowPen);
    Move(rp, x + w - 2, y + 2);
    Draw(rp, x + w - 2, y + h - 2);
    Draw(rp, x + 2, y + h - 2);
}

static void bevel_draw_group(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Recessed outer frame: shadow on top-left, shine on bottom-right */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x, y + h - 2);
    Draw(rp, x, y);
    Draw(rp, x + w - 2, y);

    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + w - 1, y);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x, y + h - 1);

    /* Inner highlight */
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + 1, y + h - 3);
    Draw(rp, x + 1, y + 1);
    Draw(rp, x + w - 3, y + 1);

    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x + w - 2, y + 1);
    Draw(rp, x + w - 2, y + h - 2);
    Draw(rp, x + 1, y + h - 2);
}

static void bevel_draw_field(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Recessed field: dark top-left, bright bottom-right */
    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);
}

static void bevel_draw_box(struct RastPort *rp, UWORD *pens,
    WORD x, WORD y, WORD w, WORD h)
{
    /* Raised box: shine top-left, shadow bottom-right */
    SetAPen(rp, pens[SHINEPEN]);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, pens[SHADOWPEN]);
    Move(rp, x + w - 1, y + 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + 1, y + h - 1);
}

/******************************************************************************/

/* Snapshot the cap_BevelType prefs value (default BVT_GT). */
static UBYTE bevel_read_prefs_visual(void)
{
    struct UIPrefs *prefs;
    UBYTE vt = BVT_GT;

    prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
    if (prefs)
    {
        ObtainSemaphoreShared(&prefs->cap_Semaphore);
        vt = prefs->cap_BevelType;
        ReleaseSemaphore(&prefs->cap_Semaphore);
    }
    return vt;
}

/******************************************************************************/

IPTR Bevel__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        D(bug("[Bevel] OM_NEW: obj 0x%p\n", (APTR)retval));
        struct BevelData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct BevelData));
        data->bd_Style = BVS_GROUP;
        data->bd_VisualType = bevel_read_prefs_visual();

        bevel_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Bevel__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Bevel] OM_DISPOSE: obj 0x%p\n", o));
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Bevel__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Bevel] OM_SET: obj 0x%p\n", o));
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    bevel_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Bevel__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    UWORD t = bevel_thickness_for(data->bd_Style, data->bd_VisualType);

    switch (msg->opg_AttrID)
    {
        case BEVEL_Style:
            *msg->opg_Storage = data->bd_Style;
            return TRUE;

        case BEVEL_Label:
            *msg->opg_Storage = (IPTR)data->bd_Label;
            return TRUE;

        case BEVEL_LabelPlace:
            *msg->opg_Storage = data->bd_LabelPlace;
            return TRUE;

        case BEVEL_TextPen:
            *msg->opg_Storage = data->bd_TextPen;
            return TRUE;

        case BEVEL_FillPen:
            *msg->opg_Storage = data->bd_FillPen;
            return TRUE;

        case BEVEL_Transparent:
            *msg->opg_Storage = data->bd_Transparent;
            return TRUE;

        case BEVEL_HorizSize:
            *msg->opg_Storage = t;
            return TRUE;

        case BEVEL_VertSize:
            *msg->opg_Storage = t;
            return TRUE;

        case BEVEL_InnerLeft:
            *msg->opg_Storage = im->LeftEdge + t;
            return TRUE;

        case BEVEL_InnerTop:
            *msg->opg_Storage = im->TopEdge + t;
            return TRUE;

        case BEVEL_InnerWidth:
            *msg->opg_Storage = (im->Width  > 2 * t) ? im->Width  - 2 * t : 0;
            return TRUE;

        case BEVEL_InnerHeight:
            *msg->opg_Storage = (im->Height > 2 * t) ? im->Height - 2 * t : 0;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

/* Shared rendering core: draw a bevel of the given style at (x,y) with the
 * given width/height. Used by both IM_DRAW (which uses the image's own
 * LeftEdge/Width/...) and IM_DRAWFRAME (caller supplies size). */
static void bevel_render(struct BevelData *data, struct RastPort *rp,
    struct DrawInfo *dri, ULONG state, WORD x, WORD y, WORD w, WORD h)
{
    UWORD *pens = dri->dri_Pens;
    UWORD t;

    if (w < 2 || h < 2)
        return;

    /* Erase the interior unless the caller explicitly asked us not to.
     * Selected state uses FILLPEN to give pressed/active feedback; the
     * BEVEL_FillPen attribute (when set) overrides the default. */
    if (!data->bd_Transparent && data->bd_Style != BVS_NONE)
    {
        UWORD pen;
        WORD ix, iy, iw, ih;

        if (data->bd_FillPenSet)
            pen = data->bd_FillPen;
        else if (state == IDS_SELECTED)
            pen = pens[FILLPEN];
        else
            pen = pens[BACKGROUNDPEN];

        t = bevel_thickness_for(data->bd_Style, data->bd_VisualType);
        ix = x + t;
        iy = y + t;
        iw = w - 2 * t;
        ih = h - 2 * t;

        if (iw > 0 && ih > 0)
        {
            SetAPen(rp, pen);
            SetDrMd(rp, JAM1);
            RectFill(rp, ix, iy, ix + iw - 1, iy + ih - 1);
        }
    }
    else if (!data->bd_Transparent && data->bd_Style == BVS_NONE)
    {
        /* No frame at all but caller still wants a clean interior */
        UWORD pen = data->bd_FillPenSet ? data->bd_FillPen
                                        : pens[BACKGROUNDPEN];
        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
    }

    /* For thin visual prefs, force 1px style equivalents regardless of the
     * caller's BVS_* selection. Raised styles fall through to the BOX/THIN
     * routines that draw raised vs recessed appearance with a single line. */
    if (data->bd_VisualType == BVT_THIN || data->bd_VisualType == BVT_XENTHIN)
    {
        switch (data->bd_Style)
        {
            case BVS_NONE:
                break;
            case BVS_BUTTON:
            case BVS_BOX:
            case BVS_STANDARD:
                bevel_draw_box(rp, pens, x, y, w, h);
                break;
            default:
                bevel_draw_thin(rp, pens, x, y, w, h);
                break;
        }
    }
    else switch (data->bd_Style)
    {
        case BVS_NONE:
            break;
        case BVS_THIN:
            bevel_draw_thin(rp, pens, x, y, w, h);
            break;
        case BVS_BUTTON:
        case BVS_STANDARD:
            bevel_draw_button(rp, pens, x, y, w, h, state);
            break;
        case BVS_GROUP:
            bevel_draw_group(rp, pens, x, y, w, h);
            break;
        case BVS_FIELD:
        case BVS_DROPBOX:
            bevel_draw_field(rp, pens, x, y, w, h);
            break;
        case BVS_BOX:
            bevel_draw_box(rp, pens, x, y, w, h);
            break;
        case BVS_SBAR_VERT:
        case BVS_SBAR_HORIZ:
            bevel_draw_thin(rp, pens, x, y, w, h);
            break;
        default:
            bevel_draw_group(rp, pens, x, y, w, h);
            break;
    }

    /* Draw label if present (for group-style bevels) */
    if (data->bd_Label && data->bd_Style == BVS_GROUP)
    {
        UWORD pen = data->bd_TextPen ? data->bd_TextPen : pens[TEXTPEN];
        WORD textX = x + 8;
        WORD textY = y;
        ULONG len = strlen(data->bd_Label);

        SetAPen(rp, pens[BACKGROUNDPEN]);
        RectFill(rp, textX - 2, textY - 1,
            textX + TextLength(rp, data->bd_Label, len) + 1,
            textY + rp->TxHeight - 1);

        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        Move(rp, textX, textY + rp->TxBaseline);
        Text(rp, data->bd_Label, len);
    }
}

/******************************************************************************/

IPTR Bevel__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    WORD x, y, w, h;

    D(bug("[Bevel] IM_DRAW: state %d, dimensions %dx%d, style %d\n",
        msg->imp_State, im->Width, im->Height, data->bd_Style));

    if (!rp || !dri)
        return FALSE;

    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    bevel_render(data, rp, dri, msg->imp_State, x, y, w, h);
    return TRUE;
}

/******************************************************************************/

/* IM_DRAWFRAME - draw with caller-supplied size (msg->imp_Dimensions). */
IPTR Bevel__IM_DRAWFRAME(Class *cl, Object *o, struct impDraw *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    WORD x, y, w, h;

    if (!rp || !dri)
        return FALSE;

    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = msg->imp_Dimensions.Width;
    h = msg->imp_Dimensions.Height;

    D(bug("[Bevel] IM_DRAWFRAME: state %d, %dx%d, style %d\n",
        msg->imp_State, w, h, data->bd_Style));

    bevel_render(data, rp, dri, msg->imp_State, x, y, w, h);
    return TRUE;
}

/******************************************************************************/

/* IM_FRAMEBOX - given a contents IBox, return the enclosing frame IBox.
 * A bevel of thickness T expands the contents by T pixels on every side. */
IPTR Bevel__IM_FRAMEBOX(Class *cl, Object *o, struct impFrameBox *msg)
{
    struct BevelData *data = INST_DATA(cl, o);
    struct IBox *cb = msg->imp_ContentsBox;
    struct IBox *fb = msg->imp_FrameBox;
    UWORD t;

    if (!cb || !fb)
        return FALSE;

    t = bevel_thickness_for(data->bd_Style, data->bd_VisualType);

    fb->Left   = cb->Left   - t;
    fb->Top    = cb->Top    - t;
    fb->Width  = cb->Width  + 2 * t;
    fb->Height = cb->Height + 2 * t;

    /* Group-style bevels with a label need a little extra top padding so the
     * label sits cleanly above the contents. */
    if (data->bd_Style == BVS_GROUP && data->bd_Label)
    {
        WORD lh = (msg->imp_DrInfo && msg->imp_DrInfo->dri_Font)
                ? msg->imp_DrInfo->dri_Font->tf_YSize : 8;
        fb->Top    -= lh / 2;
        fb->Height += lh / 2;
    }

    return TRUE;
}

/******************************************************************************/

/* IM_HITTEST - bevel images don't accept hits. */
IPTR Bevel__IM_HITTEST(Class *cl, Object *o, struct impHitTest *msg)
{
    return FALSE;
}
