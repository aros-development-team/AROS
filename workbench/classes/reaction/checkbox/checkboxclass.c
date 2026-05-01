/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction checkbox.gadget - BOOPSI class implementation
*/

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
#include <gadgets/checkbox.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>

#include <string.h>

#include "checkbox_intern.h"

#define CheckboxBase ((struct Library *)(cl->cl_UserData))

#define CHECKBOX_WIDTH  26
#define CHECKBOX_HEIGHT 11

/******************************************************************************/

static void checkbox_set(Class *cl, Object *o, struct opSet *msg)
{
    struct CheckboxData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case GA_Selected:
                data->cd_Checked = (BOOL)tag->ti_Data;
                break;
            case CHECKBOX_TextPen:
                data->cd_TextPen = (UWORD)tag->ti_Data;
                break;
            case CHECKBOX_BackgroundPen:
                data->cd_BackgroundPen = (UWORD)tag->ti_Data;
                break;
            case CHECKBOX_FillTextPen:
                data->cd_FillTextPen = (UWORD)tag->ti_Data;
                break;
            case CHECKBOX_TextPlace:
                data->cd_TextPlace = tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR CheckBox__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct CheckboxData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct CheckboxData));

        checkbox_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR CheckBox__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR CheckBox__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    checkbox_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR CheckBox__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct CheckboxData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case GA_Selected:
            *msg->opg_Storage = data->cd_Checked;
            return TRUE;

        case CHECKBOX_TextPen:
            *msg->opg_Storage = data->cd_TextPen;
            return TRUE;

        case CHECKBOX_BackgroundPen:
            *msg->opg_Storage = data->cd_BackgroundPen;
            return TRUE;

        case CHECKBOX_FillTextPen:
            *msg->opg_Storage = data->cd_FillTextPen;
            return TRUE;

        case CHECKBOX_TextPlace:
            *msg->opg_Storage = data->cd_TextPlace;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

static void checkbox_render_box(struct RastPort *rp, struct DrawInfo *dri,
                                WORD x, WORD y, WORD w, WORD h, BOOL checked)
{
    UWORD shine = dri ? dri->dri_Pens[SHINEPEN] : 2;
    UWORD shadow = dri ? dri->dri_Pens[SHADOWPEN] : 1;
    UWORD bg = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
    UWORD fill = dri ? dri->dri_Pens[FILLPEN] : 3;

    /* Draw recessed box frame */
    SetAPen(rp, shadow);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - 1, y);

    SetAPen(rp, shine);
    Move(rp, x + 1, y + h - 1);
    Draw(rp, x + w - 1, y + h - 1);
    Draw(rp, x + w - 1, y + 1);

    /* Fill interior */
    SetAPen(rp, bg);
    RectFill(rp, x + 1, y + 1, x + w - 2, y + h - 2);

    /* Draw checkmark if checked */
    if (checked)
    {
        SetAPen(rp, fill);
        /* Simple X-style checkmark */
        Move(rp, x + 3, y + 2);
        Draw(rp, x + w - 4, y + h - 3);
        Move(rp, x + w - 4, y + 2);
        Draw(rp, x + 3, y + h - 3);
        /* Thicken the mark */
        Move(rp, x + 4, y + 2);
        Draw(rp, x + w - 3, y + h - 3);
        Move(rp, x + w - 3, y + 2);
        Draw(rp, x + 4, y + h - 3);
    }
}

IPTR CheckBox__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct CheckboxData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    WORD bx, by, bw, bh;

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    /* Clear gadget area */
    SetAPen(rp, dri ? dri->dri_Pens[BACKGROUNDPEN] : 0);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Center the checkbox box vertically */
    bw = CHECKBOX_WIDTH;
    bh = CHECKBOX_HEIGHT;
    if (bw > w) bw = w;
    if (bh > h) bh = h;
    bx = x;
    by = y + (h - bh) / 2;

    checkbox_render_box(rp, dri, bx, by, bw, bh, data->cd_Checked);

    /* Draw label text from gadget text */
    if (gad->GadgetText)
    {
        UWORD pen = data->cd_TextPen ? data->cd_TextPen :
                    (dri ? dri->dri_Pens[TEXTPEN] : 1);
        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        Move(rp, bx + bw + 6, by + rp->TxBaseline);
        Text(rp, gad->GadgetText->IText,
             strlen(gad->GadgetText->IText));
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

/******************************************************************************/

IPTR CheckBox__GM_GOACTIVE(Class *cl, Object *o, struct gpInput *msg)
{
    struct CheckboxData *data = INST_DATA(cl, o);
    struct Gadget *gad = G(o);

    if (gad->Flags & GFLG_DISABLED)
        return GMR_NOREUSE;

    /* Toggle checked state */
    data->cd_Checked = !data->cd_Checked;

    /* Request a redraw */
    struct RastPort *rp = ObtainGIRPort(msg->gpi_GInfo);
    if (rp)
    {
        struct gpRender render;
        render.MethodID   = GM_RENDER;
        render.gpr_GInfo  = msg->gpi_GInfo;
        render.gpr_RPort  = rp;
        render.gpr_Redraw = GREDRAW_UPDATE;
        DoMethodA(o, (Msg)&render);
        ReleaseGIRPort(rp);
    }

    /* Notify - checkboxes toggle immediately */
    if (msg->gpi_IEvent)
    {
        *msg->gpi_Termination = data->cd_Checked;
    }

    return GMR_NOREUSE | GMR_VERIFY;
}

/******************************************************************************/

IPTR CheckBox__GM_HANDLEINPUT(Class *cl, Object *o, struct gpInput *msg)
{
    return GMR_MEACTIVE;
}
