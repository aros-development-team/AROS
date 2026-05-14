/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction button.gadget - BOOPSI class implementation
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
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/button.h>
#include <images/bevel.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

#include <string.h>

#include "button_intern.h"

#define ButtonBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void button_set(Class *cl, Object *o, struct opSet *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case BUTTON_PushButton:
                data->bd_PushButton = (BOOL)tag->ti_Data;
                break;
            case BUTTON_Glyph:
                data->bd_Glyph = (Object *)tag->ti_Data;
                break;
            case BUTTON_AutoButton:
                data->bd_AutoButton = tag->ti_Data;
                break;
            case BUTTON_BevelStyle:
                if (data->bd_BevelStyle != tag->ti_Data && data->bd_BevelImage)
                {
                    DisposeObject(data->bd_BevelImage);
                    data->bd_BevelImage = NULL;
                }
                data->bd_BevelStyle = tag->ti_Data;
                break;
            case BUTTON_Justification:
                data->bd_Justification = tag->ti_Data;
                break;
            case BUTTON_SoftStyle:
                data->bd_SoftStyle = tag->ti_Data;
                break;
            case BUTTON_TextPen:
                data->bd_TextPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_BackgroundPen:
                data->bd_BackgroundPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_FillTextPen:
                data->bd_FillTextPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_FillPen:
                data->bd_FillPen = (UWORD)tag->ti_Data;
                break;
            case BUTTON_Transparent:
                data->bd_Transparent = (BOOL)tag->ti_Data;
                break;
            case BUTTON_DomainString:
                data->bd_DomainString = (STRPTR)tag->ti_Data;
                break;
            case GA_Underscore:
                data->bd_Underscore = (UBYTE)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

/* Render a string handling the GA_Underscore prefix character. The character
 * following the prefix is drawn underlined. A doubled prefix (e.g. "__") is
 * collapsed to a single literal occurrence of the prefix character. */
static void button_draw_text(struct RastPort *rp, STRPTR text, ULONG len,
    WORD x, WORD y, UBYTE underscore)
{
    if (!text || len == 0)
        return;

    if (!underscore)
    {
        Move(rp, x, y);
        Text(rp, text, len);
        return;
    }

    {
        WORD cx = x;
        ULONG i;

        for (i = 0; i < len; i++)
        {
            if (text[i] == underscore && (i + 1) < len)
            {
                if (text[i + 1] == underscore)
                {
                    /* Doubled prefix → literal */
                    Move(rp, cx, y);
                    Text(rp, &text[i + 1], 1);
                    cx += TextLength(rp, &text[i + 1], 1);
                    i++;
                    continue;
                }

                /* Skip prefix and underline next char */
                i++;
                Move(rp, cx, y);
                Text(rp, &text[i], 1);
                {
                    WORD charW = TextLength(rp, &text[i], 1);
                    Move(rp, cx, y + 1);
                    Draw(rp, cx + charW - 1, y + 1);
                    cx += charW;
                }
            }
            else
            {
                Move(rp, cx, y);
                Text(rp, &text[i], 1);
                cx += TextLength(rp, &text[i], 1);
            }
        }
    }
}

/* Compute the on-screen pixel width of a string after stripping single
 * occurrences of the underscore prefix character. */
static ULONG button_text_pixel_width(struct RastPort *rp, STRPTR text,
    ULONG len, UBYTE underscore)
{
    ULONG pixelWidth = 0;
    ULONG i;

    if (!underscore)
        return TextLength(rp, text, len);

    for (i = 0; i < len; i++)
    {
        if (text[i] == underscore && (i + 1) < len)
        {
            if (text[i + 1] == underscore)
            {
                /* Doubled prefix counts as one visible character */
                pixelWidth += TextLength(rp, &text[i + 1], 1);
                i++;
                continue;
            }
            /* Single prefix is invisible; following char will be measured
             * on the next iteration. */
            continue;
        }
        pixelWidth += TextLength(rp, &text[i], 1);
    }
    return pixelWidth;
}

/******************************************************************************/

IPTR Button__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[Button] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[Button] OM_NEW: obj=%p\n", (void *)retval));
    if (retval)
    {
        struct ButtonData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct ButtonData));
        data->bd_Justification = BCJ_CENTER;
        data->bd_Underscore = '_';
        /* BVS_BUTTON is ClassAct/ReAction's default for button.gadget. The
         * UIPrefs cap_BevelType (BVT_*) controls the *visual* rendering
         * style of any bevel, not which kind of bevel a class chooses, so
         * it must not override BUTTON_BevelStyle here. */
        data->bd_BevelStyle = BVS_BUTTON;

        /* Snapshot label pen from prefs */
        {
            struct UIPrefs *prefs;
            prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
            if (prefs)
            {
                ObtainSemaphoreShared(&prefs->cap_Semaphore);
                data->bd_PrefsLabelPen = prefs->cap_LabelPen;
                ReleaseSemaphore(&prefs->cap_Semaphore);
            }
        }

        button_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Button__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Button] OM_DISPOSE: obj=%p\n", (void *)o));

    struct ButtonData *data = INST_DATA(cl, o);

    if (data->bd_Glyph)
    {
        DisposeObject(data->bd_Glyph);
        data->bd_Glyph = NULL;
    }
    if (data->bd_BevelImage)
    {
        DisposeObject(data->bd_BevelImage);
        data->bd_BevelImage = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Button__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Button] OM_SET: obj=%p\n", (void *)o));

    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    button_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Button__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case BUTTON_PushButton:
            *msg->opg_Storage = data->bd_PushButton;
            return TRUE;

        case BUTTON_Justification:
            *msg->opg_Storage = data->bd_Justification;
            return TRUE;

        case BUTTON_AutoButton:
            *msg->opg_Storage = data->bd_AutoButton;
            return TRUE;

        case BUTTON_BevelStyle:
            *msg->opg_Storage = data->bd_BevelStyle;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    D(bug("[Button] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

    struct ButtonData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    BOOL selected, disabled;
    UWORD state;
    STRPTR text;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);
    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;
    selected = (gad->Flags & GFLG_SELECTED) || data->bd_PushButton;
    disabled = (gad->Flags & GFLG_DISABLED) != 0;
    state = disabled ? IDS_DISABLED : (selected ? IDS_SELECTED : IDS_NORMAL);

    /* Lazily create a private bevel.image for the frame. We render via
     * IM_DRAWFRAME so a single bevel object can be reused across resizes.
     * BEVEL_Transparent is forwarded so the caller's bd_Transparent
     * preference controls whether the interior is erased. */
    if (!data->bd_BevelImage && data->bd_BevelStyle != BVS_NONE)
    {
        data->bd_BevelImage = NewObject(NULL, "bevel.image",
            BEVEL_Style,       data->bd_BevelStyle,
            BEVEL_Transparent, data->bd_Transparent,
            TAG_END);
    }

    /* Draw bevel frame (which also handles interior fill when not transparent) */
    if (data->bd_BevelImage && dri)
    {
        struct impDraw idmsg;
        idmsg.MethodID         = IM_DRAWFRAME;
        idmsg.imp_RPort        = rp;
        idmsg.imp_Offset.X     = x;
        idmsg.imp_Offset.Y     = y;
        idmsg.imp_State        = state;
        idmsg.imp_DrInfo       = dri;
        idmsg.imp_Dimensions.Width  = w;
        idmsg.imp_Dimensions.Height = h;
        DoMethodA(data->bd_BevelImage, (Msg)&idmsg);
    }

    /* Determine text content (BOOPSI gadgetclass stores GA_Text as STRPTR
     * directly in GadgetText). */
    text = data->bd_DomainString
         ? data->bd_DomainString
         : (gad->GadgetText ? (STRPTR)gad->GadgetText : NULL);

    if (text && dri)
    {
        struct TextFont *font = dri->dri_Font;
        UWORD pen;
        if (data->bd_TextPen)
            pen = data->bd_TextPen;
        else if (disabled)
            pen = dri->dri_Pens[SHADOWPEN];
        else if (selected)
            pen = dri->dri_Pens[FILLTEXTPEN];
        else if (data->bd_PrefsLabelPen)
            pen = data->bd_PrefsLabelPen;
        else
            pen = dri->dri_Pens[TEXTPEN];
        ULONG len = strlen(text);
        struct TextExtent te;
        WORD tx, ty;
        WORD textPixelW;
        struct TextFont *prevFont = NULL;

        if (font)
        {
            prevFont = rp->Font;
            SetFont(rp, font);
        }
        TextExtent(rp, text, len, &te);
        textPixelW = button_text_pixel_width(rp, text, len, data->bd_Underscore);

        switch (data->bd_Justification)
        {
            case BCJ_LEFT:   tx = x + 4; break;
            case BCJ_RIGHT:  tx = x + w - textPixelW - 4; break;
            case BCJ_CENTER:
            default:         tx = x + (w - textPixelW) / 2; break;
        }
        ty = y + (h - te.te_Height) / 2 + rp->TxBaseline;

        if (selected)
        {
            tx += 1;
            ty += 1;
        }

        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        button_draw_text(rp, text, len, tx, ty, data->bd_Underscore);

        if (prevFont)
            SetFont(rp, prevFont);
    }

    /* Draw glyph image if present (auto-button) */
    if (data->bd_Glyph && dri)
    {
        DrawImageState(rp, (struct Image *)data->bd_Glyph,
            x + (w - ((struct Image *)data->bd_Glyph)->Width) / 2,
            y + (h - ((struct Image *)data->bd_Glyph)->Height) / 2,
            state, dri);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}

/******************************************************************************/

IPTR Button__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[Button] GM_GOACTIVE: obj=%p\n", (void *)o));

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    D(bug("[Button] GM_HANDLEINPUT: obj=%p\n", (void *)o));

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_GOINACTIVE(Class *cl, Object *o, struct gpGoInactive *msg)
{
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Button__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct ButtonData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);
    STRPTR text = NULL;
    UWORD minW = 40, minH = 14;
    struct TextFont *font = NULL;

    /* BOOPSI gadgetclass stores GA_Text directly as STRPTR. */
    if (data->bd_DomainString)
        text = data->bd_DomainString;
    else if (gad->GadgetText)
        text = (STRPTR)gad->GadgetText;

    /* Prefer the UI font from DrawInfo - the screen RastPort's Font may
     * point at GfxBase->DefaultFont (huge) rather than the screen font. */
    if (msg->gpd_GInfo && msg->gpd_GInfo->gi_DrInfo)
        font = msg->gpd_GInfo->gi_DrInfo->dri_Font;
    if (!font && msg->gpd_RPort)
        font = msg->gpd_RPort->Font;

    if (text && font)
    {
        struct RastPort rp;
        struct TextExtent te;
        UWORD bx = 2, by = 2;
        ULONG textPixelW;

        InitRastPort(&rp);
        SetFont(&rp, font);
        TextExtent(&rp, text, strlen(text), &te);
        textPixelW = button_text_pixel_width(&rp, text, strlen(text),
            data->bd_Underscore);

        /* Pad by twice the bevel thickness (one each side) plus a couple of
         * pixels for breathing room. */
        switch (data->bd_BevelStyle)
        {
            case BVS_NONE: bx = by = 0; break;
            case BVS_THIN: case BVS_BOX: case BVS_FOCUS: bx = by = 1; break;
            default: bx = by = 2; break;
        }
        minW = textPixelW    + 2 * bx + 12;
        minH = te.te_Height  + 2 * by + 4;
    }
    else if (text && msg->gpd_RPort)
    {
        ULONG textPixelW = button_text_pixel_width(msg->gpd_RPort, text,
            strlen(text), data->bd_Underscore);
        struct TextExtent te;
        TextExtent(msg->gpd_RPort, text, strlen(text), &te);
        minW = textPixelW   + 16;
        minH = te.te_Height + 6;
    }

    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = minW;
    msg->gpd_Domain.Height = minH;

    D(bug("[Button] GM_DOMAIN: obj=%p text='%s' font=%p teH=%d -> %dx%d\n",
        o, text ? text : (STRPTR)"(null)", font,
        font ? (int)font->tf_YSize : 0, (int)minW, (int)minH));

    return TRUE;
}
