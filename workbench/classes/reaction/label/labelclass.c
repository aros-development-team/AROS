/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction label.image - BOOPSI class implementation
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
#include <images/label.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

#include <string.h>

#include "label_intern.h"

#define LabelBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void label_set(Class *cl, Object *o, struct opSet *msg)
{
    struct LabelData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case LABEL_Text:
                data->ld_Text = (STRPTR)tag->ti_Data;
                break;
            case LABEL_Image:
                data->ld_Image = (Object *)tag->ti_Data;
                break;
            case LABEL_Justification:
                data->ld_Justification = tag->ti_Data;
                break;
            case LABEL_SoftStyle:
                data->ld_SoftStyle = tag->ti_Data;
                break;
            case LABEL_DisposeImage:
                data->ld_DisposeImage = (BOOL)tag->ti_Data;
                break;
            case LABEL_Mapping:
                data->ld_Mapping = (LONG *)tag->ti_Data;
                break;
            case LABEL_DrawInfo:
                data->ld_DrawInfo = (struct DrawInfo *)tag->ti_Data;
                break;
            case LABEL_Underscore:
                data->ld_Underscore = (UBYTE)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void label_draw_text(struct RastPort *rp, STRPTR text, ULONG len,
    WORD x, WORD y, UBYTE underscore)
{
    if (!text || len == 0)
        return;

    if (underscore)
    {
        ULONG i;
        WORD cx = x;

        for (i = 0; i < len; i++)
        {
            if (text[i] == underscore && (i + 1) < len)
            {
                /* Draw text before underscore char */
                if (i > 0)
                {
                    /* Partial text already drawn, advance from cx */
                }

                /* Skip the underscore character itself */
                i++;

                /* Draw the underlined character */
                Move(rp, cx, y);
                Text(rp, &text[i], 1);

                {
                    WORD charW = TextLength(rp, &text[i], 1);
                    /* Draw underline beneath the character */
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
    else
    {
        Move(rp, x, y);
        Text(rp, text, len);
    }
}

/* Calculate text length excluding the underscore prefix character */
static ULONG label_text_pixel_width(struct RastPort *rp, STRPTR text,
    ULONG len, UBYTE underscore)
{
    ULONG pixelWidth;

    if (!underscore)
        return TextLength(rp, text, len);

    pixelWidth = 0;
    {
        ULONG i;
        for (i = 0; i < len; i++)
        {
            if (text[i] == underscore && (i + 1) < len)
            {
                /* Skip the underscore prefix character itself */
                continue;
            }
            pixelWidth += TextLength(rp, &text[i], 1);
        }
    }

    return pixelWidth;
}

/******************************************************************************/

IPTR Label__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        D(bug("[Label] OM_NEW: obj 0x%p\n", (APTR)retval));
        struct LabelData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct LabelData));
        data->ld_Justification = LJ_CENTER;
        data->ld_Underscore = '_';

        /* Snapshot label-related prefs */
        {
            struct UIPrefs *prefs;
            prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
            if (prefs)
            {
                ObtainSemaphoreShared(&prefs->cap_Semaphore);
                data->ld_PrefsLabelPen = prefs->cap_LabelPen;
                data->ld_3DLabel       = prefs->cap_3DLabel ? TRUE : FALSE;
                ReleaseSemaphore(&prefs->cap_Semaphore);
            }
        }

        label_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Label__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[Label] OM_DISPOSE: obj 0x%p\n", o));
    struct LabelData *data = INST_DATA(cl, o);

    if (data->ld_DisposeImage && data->ld_Image)
    {
        DisposeObject(data->ld_Image);
        data->ld_Image = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR Label__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[Label] OM_SET: obj 0x%p\n", o));
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    label_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Label__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct LabelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;

    switch (msg->opg_AttrID)
    {
        /* Compute on-demand size based on text + optional image. The image
         * struct fields are 0 unless someone explicitly sets IA_Width/Height
         * via OM_SET — but callers (e.g. layout) typically just GetAttr
         * IA_Width to sample minimum width. Synthesise it from the text. */
        case IA_Width:
        {
            ULONG w = 0;
            /* Compute natural width from text + sub-image. Do NOT trust
             * im->Width as a "set" indicator: imageclass initialises it to
             * a default 80x40 unless overridden, so checking it first will
             * mask the real text-derived size. */
            if (data->ld_Text && data->ld_Text[0])
            {
                struct Screen *scr = LockPubScreen(NULL);
                if (scr)
                {
                    struct RastPort rp;
                    InitRastPort(&rp);
                    SetFont(&rp, scr->RastPort.Font);
                    w = label_text_pixel_width(&rp, data->ld_Text,
                        strlen(data->ld_Text), data->ld_Underscore);
                    UnlockPubScreen(NULL, scr);
                }
            }
            if (data->ld_Image)
            {
                IPTR iw = 0;
                GetAttr(IA_Width, data->ld_Image, &iw);
                if ((ULONG)iw > w) w = (ULONG)iw;
            }
            /* Fall back to image record only if we have neither text nor
             * a sub-image (e.g. unused spacer label). */
            if (w == 0 && !data->ld_Text && !data->ld_Image)
                w = im->Width;
            *msg->opg_Storage = w;
            D(bug("[Label] OM_GET: obj %p IA_Width -> %ld (text='%s')\n",
                o, (LONG)w, data->ld_Text ? data->ld_Text : (STRPTR)""));
            return TRUE;
        }

        case IA_Height:
        {
            ULONG h = 0;
            if (data->ld_Text && data->ld_Text[0])
            {
                struct Screen *scr = LockPubScreen(NULL);
                if (scr)
                {
                    h = scr->RastPort.Font ? scr->RastPort.Font->tf_YSize : 8;
                    UnlockPubScreen(NULL, scr);
                }
            }
            if (data->ld_Image)
            {
                IPTR ih = 0;
                GetAttr(IA_Height, data->ld_Image, &ih);
                if ((ULONG)ih > h) h = (ULONG)ih;
            }
            if (h == 0 && !data->ld_Text && !data->ld_Image)
                h = im->Height;
            *msg->opg_Storage = h;
            D(bug("[Label] OM_GET: obj %p IA_Height -> %ld (text='%s')\n",
                o, (LONG)h, data->ld_Text ? data->ld_Text : (STRPTR)""));
            return TRUE;
        }

        case LABEL_Text:
            *msg->opg_Storage = (IPTR)data->ld_Text;
            return TRUE;

        case LABEL_Image:
            *msg->opg_Storage = (IPTR)data->ld_Image;
            return TRUE;

        case LABEL_Justification:
            *msg->opg_Storage = data->ld_Justification;
            return TRUE;

        case LABEL_SoftStyle:
            *msg->opg_Storage = data->ld_SoftStyle;
            return TRUE;

        case LABEL_DisposeImage:
            *msg->opg_Storage = data->ld_DisposeImage;
            return TRUE;

        case LABEL_Mapping:
            *msg->opg_Storage = (IPTR)data->ld_Mapping;
            return TRUE;

        case LABEL_DrawInfo:
            *msg->opg_Storage = (IPTR)data->ld_DrawInfo;
            return TRUE;

        case LABEL_Underscore:
            *msg->opg_Storage = data->ld_Underscore;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR Label__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct LabelData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    UWORD *pens;
    WORD x, y, w, h;
    UWORD textPen;
    ULONG oldStyle;

    D(bug("[Label] IM_DRAW: state %d, dimensions %dx%d, text '%s'\n", msg->imp_State, im->Width, im->Height, data->ld_Text ? data->ld_Text : (STRPTR)"(null)"));

    if (!rp)
        return FALSE;

    /* Use embedded DrawInfo if msg doesn't provide one */
    if (!dri)
        dri = data->ld_DrawInfo;
    if (!dri)
        return FALSE;

    pens = dri->dri_Pens;
    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    /* Select text pen. Order: explicit override (none in public API), then
     * UIPrefs cap_LabelPen, then DrawInfo state-derived. Disabled state
     * always uses SHADOWPEN regardless of overrides. */
    switch (msg->imp_State)
    {
        case IDS_SELECTED:
        case IDS_INACTIVESELECTED:
            textPen = pens[FILLTEXTPEN];
            break;

        case IDS_DISABLED:
        case IDS_INACTIVEDISABLED:
            textPen = pens[SHADOWPEN];
            break;

        default:
            if (data->ld_PrefsLabelPen)
                textPen = data->ld_PrefsLabelPen;
            else
                textPen = pens[TEXTPEN];
            break;
    }

    SetDrMd(rp, JAM1);

    /* Draw the optional image first */
    if (data->ld_Image)
    {
        DrawImageState(rp, (struct Image *)data->ld_Image,
            x, y, msg->imp_State, dri);
    }

    /* Draw text label */
    if (data->ld_Text)
    {
        ULONG len = strlen(data->ld_Text);
        WORD textX, textY;
        ULONG pixelWidth;
        struct TextFont *oldFont = NULL;
        struct TextFont *useFont = NULL;

        /* Make sure a font is selected. The rastport handed to us by
         * DrawImageState/layout may not have one set, in which case
         * TxHeight/TxBaseline/TextLength all return 0 and the text
         * renders at an incorrect Y position or with zero width. */
        if (!rp->Font)
        {
            struct Screen *scr = LockPubScreen(NULL);
            if (scr)
            {
                useFont = scr->RastPort.Font;
                if (useFont)
                {
                    oldFont = rp->Font;
                    SetFont(rp, useFont);
                }
                UnlockPubScreen(NULL, scr);
            }
        }

        /* Apply soft style */
        oldStyle = SetSoftStyle(rp, data->ld_SoftStyle, AskSoftStyle(rp));

        pixelWidth = label_text_pixel_width(rp, data->ld_Text, len,
            data->ld_Underscore);

        /* Calculate text Y position (vertically centered).
         * Use a sensible fallback if the font reports zero height. */
        {
            WORD th = rp->TxHeight ? rp->TxHeight : 8;
            WORD tb = rp->TxBaseline ? rp->TxBaseline : (th - 1);
            textY = y + (h - th) / 2 + tb;
        }

        /* Calculate text X position based on justification */
        switch (data->ld_Justification)
        {
            case LJ_RIGHT:
                textX = x + w - pixelWidth;
                break;

            case LJ_CENTER:
                textX = x + (w - pixelWidth) / 2;
                break;

            case LJ_LEFT:
            default:
                textX = x;
                break;
        }

        SetAPen(rp, textPen);
        /* 3D label: draw a SHINEPEN highlight one pixel down/right behind
         * the main text for an embossed look (only for normal, non-disabled
         * states - disabled already handles its own visual). */
        if (data->ld_3DLabel
            && msg->imp_State != IDS_DISABLED
            && msg->imp_State != IDS_INACTIVEDISABLED)
        {
            SetAPen(rp, pens[SHINEPEN]);
            label_draw_text(rp, data->ld_Text, len, textX + 1, textY + 1,
                data->ld_Underscore);
            SetAPen(rp, textPen);
        }
        label_draw_text(rp, data->ld_Text, len, textX, textY,
            data->ld_Underscore);

        /* Restore soft style */
        SetSoftStyle(rp, oldStyle, AskSoftStyle(rp));

        /* Restore previous font if we changed it */
        if (oldFont)
            SetFont(rp, oldFont);
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
