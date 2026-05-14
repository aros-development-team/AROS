/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction textfield.gadget - BOOPSI class implementation
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
#include <gadgets/textfield.h>
#include <images/bevel.h>
#include <utility/tagitem.h>

#include <string.h>

#include "textfield_intern.h"

#define TextFieldBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void textfield_set(Class *cl, Object *o, struct opSet *msg)
{
    struct TextFieldData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case TEXTFIELD_Text:
                data->td_Text = (STRPTR)tag->ti_Data;
                break;
            case TEXTFIELD_MaxChars:
                data->td_MaxChars = (ULONG)tag->ti_Data;
                break;
            case TEXTFIELD_ReadOnly:
                data->td_ReadOnly = (BOOL)tag->ti_Data;
                break;
            case TEXTFIELD_Partial:
                data->td_Partial = (BOOL)tag->ti_Data;
                break;
            case TEXTFIELD_WordWrap:
                data->td_WordWrap = (BOOL)tag->ti_Data;
                break;
            case TEXTFIELD_CursorPos:
                data->td_CursorPos = (ULONG)tag->ti_Data;
                break;
            case TEXTFIELD_Top:
                data->td_Top = (ULONG)tag->ti_Data;
                break;
            case TEXTFIELD_Blinkrate:
                data->td_Blinkrate = (ULONG)tag->ti_Data;
                break;
            case TEXTFIELD_TabSpaces:
                data->td_TabSpaces = (UWORD)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR TextField__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[TextField] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct TextFieldData *data = INST_DATA(cl, (Object *)retval);

        D(bug("[TextField] OM_NEW: obj 0x%p\n", (void *)retval));

        memset(data, 0, sizeof(struct TextFieldData));

        /* Set defaults */
        data->td_MaxChars  = 4096;
        data->td_WordWrap  = TRUE;
        data->td_TabSpaces = 8;
        data->td_Blinkrate = 20;

        textfield_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR TextField__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct TextFieldData *data = INST_DATA(cl, o);
    if (data->td_BevelImage)
    {
        DisposeObject(data->td_BevelImage);
        data->td_BevelImage = NULL;
    }
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR TextField__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    textfield_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR TextField__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct TextFieldData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case TEXTFIELD_Text:
            *msg->opg_Storage = (IPTR)data->td_Text;
            return TRUE;

        case TEXTFIELD_MaxChars:
            *msg->opg_Storage = data->td_MaxChars;
            return TRUE;

        case TEXTFIELD_ReadOnly:
            *msg->opg_Storage = data->td_ReadOnly;
            return TRUE;

        case TEXTFIELD_WordWrap:
            *msg->opg_Storage = data->td_WordWrap;
            return TRUE;

        case TEXTFIELD_CursorPos:
            *msg->opg_Storage = data->td_CursorPos;
            return TRUE;

        case TEXTFIELD_Lines:
            *msg->opg_Storage = data->td_Lines;
            return TRUE;

        case TEXTFIELD_Top:
            *msg->opg_Storage = data->td_Top;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR TextField__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct TextFieldData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;

    D(bug("[TextField] GM_RENDER: redraw 0x%lx\n", (unsigned long)msg->gpr_Redraw));

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Fill background */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Draw recessed border frame via bevel.image (BVS_FIELD = recessed). */
    if (dri)
    {
        if (!data->td_BevelImage)
        {
            data->td_BevelImage = NewObject(NULL, "bevel.image",
                BEVEL_Style, BVS_FIELD,
                BEVEL_Transparent, TRUE,
                TAG_END);
        }
        if (data->td_BevelImage)
        {
            struct impDraw idmsg;
            idmsg.MethodID         = IM_DRAWFRAME;
            idmsg.imp_RPort        = rp;
            idmsg.imp_Offset.X     = x;
            idmsg.imp_Offset.Y     = y;
            idmsg.imp_State        = IDS_NORMAL;
            idmsg.imp_DrInfo       = dri;
            idmsg.imp_Dimensions.Width  = w;
            idmsg.imp_Dimensions.Height = h;
            DoMethodA(data->td_BevelImage, (Msg)&idmsg);
        }
    }

    /* Render text content inside the border */
    if (data->td_Text && data->td_Text[0] != '\0')
    {
        WORD tx = x + 3;
        WORD ty = y + 2 + rp->TxBaseline;
        WORD maxw = w - 6;
        WORD lineheight = rp->TxHeight;
        STRPTR pos = data->td_Text;
        ULONG linenum = 0;
        ULONG topline = data->td_Top;

        SetAPen(rp, dri ? dri->dri_Pens[TEXTPEN] : 1);
        SetDrMd(rp, JAM1);

        while (*pos && (ty + lineheight - rp->TxBaseline) <= (y + h - 2))
        {
            STRPTR linestart = pos;
            ULONG linelen = 0;

            /* Find end of current line */
            while (*pos && *pos != '\n')
            {
                pos++;
                linelen++;
            }

            /* Only render lines at or past the top visible line */
            if (linenum >= topline)
            {
                ULONG drawlen = linelen;
                if (maxw > 0 && rp->TxWidth > 0)
                {
                    ULONG maxchars = maxw / rp->TxWidth;
                    if (drawlen > maxchars)
                        drawlen = maxchars;
                }
                Move(rp, tx, ty);
                Text(rp, linestart, drawlen);
                ty += lineheight;
            }

            linenum++;

            if (*pos == '\n')
                pos++;
        }

        data->td_Lines = linenum;
    }

    /* Disabled ghost pattern */
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
