/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction checkbox.gadget - BOOPSI class implementation
*/
#define DEBUG 0

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
#include <images/bevel.h>
#include <graphics/gfxmacros.h>
#include <utility/tagitem.h>
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>

#include <string.h>

#include "checkbox_intern.h"
#include "../reaction_box.h"

#define CheckboxBase ((struct Library *)(cl->cl_UserData))

#define CHECKBOX_WIDTH  26
#define CHECKBOX_HEIGHT 11
#define CHECKBOX_GAP    6

/* label text: an underscore marks the next character as the shortcut,
   which is drawn underlined; the underscore itself is not drawn */
static UWORD checkbox_text_width(struct RastPort *rp, STRPTR txt)
{
    UWORD w = 0;
    for (; *txt; txt++)
        if (*txt != '_')
            w += TextLength(rp, txt, 1);
    return w;
}

static void checkbox_draw_text(struct RastPort *rp, WORD x, WORD y, STRPTR txt)
{
    BOOL under = FALSE;
    for (; *txt; txt++)
    {
        WORD cw;
        if (*txt == '_' && !under)
        {
            under = TRUE;
            continue;
        }
        cw = TextLength(rp, txt, 1);
        Move(rp, x, y);
        Text(rp, txt, 1);
        if (under)
        {
            Move(rp, x, y + 1);
            Draw(rp, x + cw - 1, y + 1);
            under = FALSE;
        }
        x += cw;
    }
}

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

    D(bug("[CheckBox] OM_NEW: enter\n"));

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    D(bug("[CheckBox] OM_NEW: obj=%p\n", (void *)retval));
    if (retval)
    {
        struct CheckboxData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct CheckboxData));

        /* Snapshot prefs */
        {
            struct UIPrefs *prefs;
            prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
            if (prefs)
            {
                ObtainSemaphoreShared(&prefs->cap_Semaphore);
                data->cd_PrefsLabelPen = prefs->cap_LabelPen;
                data->cd_3DLabel       = prefs->cap_3DLabel ? TRUE : FALSE;
                ReleaseSemaphore(&prefs->cap_Semaphore);
            }
        }

        checkbox_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR CheckBox__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    D(bug("[CheckBox] OM_DISPOSE: obj=%p\n", (void *)o));

    struct CheckboxData *data = INST_DATA(cl, o);
    if (data->cd_BevelImage)
    {
        DisposeObject(data->cd_BevelImage);
        data->cd_BevelImage = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR CheckBox__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    D(bug("[CheckBox] OM_SET: obj=%p\n", (void *)o));

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

IPTR CheckBox__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    D(bug("[CheckBox] GM_RENDER: obj=%p redraw=%ld\n", (void *)o, msg->gpr_Redraw));

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

    reaction_gadget_box(gad, msg->gpr_GInfo, &x, &y, &w, &h);

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

    /* Draw box frame via bevel.image (BVS_FIELD = inset/recessed). */
    if (dri)
    {
        if (!data->cd_BevelImage)
        {
            data->cd_BevelImage = NewObject(NULL, "bevel.image",
                BEVEL_Style, BVS_BUTTON,
                TAG_END);
        }
        if (data->cd_BevelImage)
        {
            struct impDraw idmsg;
            idmsg.MethodID         = IM_DRAWFRAME;
            idmsg.imp_RPort        = rp;
            idmsg.imp_Offset.X     = bx;
            idmsg.imp_Offset.Y     = by;
            idmsg.imp_State        = IDS_NORMAL;
            idmsg.imp_DrInfo       = dri;
            idmsg.imp_Dimensions.Width  = bw;
            idmsg.imp_Dimensions.Height = bh;
            DoMethodA(data->cd_BevelImage, (Msg)&idmsg);
        }
    }

    /* Draw the checkmark: a tick, two pixels wide, in the text pen */
    if (data->cd_Checked && dri)
    {
        WORD l = bx + (bw - 14) / 2, t = by + (bh - 7) / 2, i;
        SetAPen(rp, dri->dri_Pens[TEXTPEN]);
        for (i = 0; i < 2; i++)
        {
            Move(rp, l + i, t + 3);
            Draw(rp, l + 3 + i, t + 6);
            Draw(rp, l + 13 + i - 1, t);
        }
    }

    /* Draw label text from gadget text (BOOPSI stores GA_Text as STRPTR) */
    if (gad->GadgetText)
    {
        STRPTR txt = (STRPTR)gad->GadgetText;
        UWORD pen;
        if (data->cd_TextPen)
            pen = data->cd_TextPen;
        else if (gad->Flags & GFLG_DISABLED)
            pen = dri ? dri->dri_Pens[SHADOWPEN] : 1;
        else if (data->cd_PrefsLabelPen)
            pen = data->cd_PrefsLabelPen;
        else
            pen = dri ? dri->dri_Pens[TEXTPEN] : 1;
        SetAPen(rp, pen);
        SetDrMd(rp, JAM1);
        if (data->cd_3DLabel && !(gad->Flags & GFLG_DISABLED) && dri)
        {
            SetAPen(rp, dri->dri_Pens[SHINEPEN]);
            checkbox_draw_text(rp, bx + bw + CHECKBOX_GAP + 1,
                               y + (h - rp->TxHeight) / 2 + rp->TxBaseline + 1, txt);
            SetAPen(rp, pen);
        }
        checkbox_draw_text(rp, bx + bw + CHECKBOX_GAP,
                           y + (h - rp->TxHeight) / 2 + rp->TxBaseline, txt);
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
    D(bug("[CheckBox] GM_GOACTIVE: obj=%p\n", (void *)o));

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
    D(bug("[CheckBox] GM_HANDLEINPUT: obj=%p\n", (void *)o));

    return GMR_MEACTIVE;
}

/******************************************************************************/

IPTR CheckBox__GM_DOMAIN(Class *cl, Object *o, struct gpDomain *msg)
{
    struct Gadget *gad = G(o);
    struct TextFont *font = NULL;
    UWORD w = CHECKBOX_WIDTH, h = CHECKBOX_HEIGHT;

    if (msg->gpd_GInfo && msg->gpd_GInfo->gi_DrInfo)
        font = msg->gpd_GInfo->gi_DrInfo->dri_Font;
    if (!font && msg->gpd_RPort)
        font = msg->gpd_RPort->Font;
    if (font)
    {
        if (gad->GadgetText)
        {
            struct RastPort rp;
            InitRastPort(&rp);
            SetFont(&rp, font);
            w += CHECKBOX_GAP + checkbox_text_width(&rp, (STRPTR)gad->GadgetText);
        }
        if (font->tf_YSize + 2 > h)
            h = font->tf_YSize + 2;
    }
    msg->gpd_Domain.Left   = 0;
    msg->gpd_Domain.Top    = 0;
    msg->gpd_Domain.Width  = w;
    msg->gpd_Domain.Height = h;
    return TRUE;
}
