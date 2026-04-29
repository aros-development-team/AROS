/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction label.image - BOOPSI class implementation
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
#include <images/label.h>
#include <utility/tagitem.h>

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
        struct LabelData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct LabelData));
        data->ld_Justification = LJ_CENTER;

        label_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR Label__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
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
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    label_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR Label__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct LabelData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
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

    /* Select text pen */
    switch (msg->imp_State)
    {
        case IDS_SELECTED:
        case IDS_INACTIVESELECTED:
            textPen = pens[FILLTEXTPEN];
            break;

        default:
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

        /* Apply soft style */
        oldStyle = SetSoftStyle(rp, data->ld_SoftStyle, AskSoftStyle(rp));

        pixelWidth = label_text_pixel_width(rp, data->ld_Text, len,
            data->ld_Underscore);

        /* Calculate text Y position (vertically centered) */
        textY = y + (h - rp->TxHeight) / 2 + rp->TxBaseline;

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
        label_draw_text(rp, data->ld_Text, len, textX, textY,
            data->ld_Underscore);

        /* Restore soft style */
        SetSoftStyle(rp, oldStyle, AskSoftStyle(rp));
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
