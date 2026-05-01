/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction radiobutton.gadget - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <exec/types.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <intuition/imageclass.h>
#include <gadgets/radiobutton.h>
#include <utility/tagitem.h>
#include <string.h>

#include "radiobutton_intern.h"

#define RadioButtonBase ((struct Library *)(cl->cl_UserData))

#define RADIO_RADIUS  5
#define RADIO_DIAM    (RADIO_RADIUS * 2)

/* ====================================================================== */

static void radiobutton_set(Class *cl, Object *o, struct opSet *msg)
{
    struct RadioButtonData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case RADIOBUTTON_Labels:
                data->labels = (struct List *)tag->ti_Data;
                break;

            case RADIOBUTTON_Selected:
                data->selected = (LONG)tag->ti_Data;
                break;

            case RADIOBUTTON_Spacing:
                data->spacing = (UWORD)tag->ti_Data;
                break;

            case RADIOBUTTON_LabelPlace:
                data->labelplace = (ULONG)tag->ti_Data;
                break;
        }
    }
}

/* ====================================================================== */

IPTR RadioButton__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct RadioButtonData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct RadioButtonData));

        /* Set defaults */
        data->selected    = 0;
        data->spacing     = 4;

        radiobutton_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/* ====================================================================== */

IPTR RadioButton__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    /* Labels list is owned by the caller; do not free it */
    return DoSuperMethodA(cl, o, msg);
}

/* ====================================================================== */

IPTR RadioButton__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    radiobutton_set(cl, o, msg);

    return retval;
}

/* ====================================================================== */

IPTR RadioButton__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct RadioButtonData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case RADIOBUTTON_Labels:
            *msg->opg_Storage = (IPTR)data->labels;
            break;

        case RADIOBUTTON_Selected:
            *msg->opg_Storage = (IPTR)data->selected;
            break;

        case RADIOBUTTON_Spacing:
            *msg->opg_Storage = (IPTR)data->spacing;
            break;

        case RADIOBUTTON_LabelPlace:
            *msg->opg_Storage = (IPTR)data->labelplace;
            break;

        default:
            return DoSuperMethodA(cl, o, (Msg)msg);
    }

    return 1;
}

/* ====================================================================== */

IPTR RadioButton__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct RadioButtonData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo->gi_DrInfo;
    struct Gadget *gad = G(o);
    UWORD *pens;
    struct Node *node;
    LONG index;
    WORD x, y;
    WORD cx, cy;
    WORD itemw, itemh;

    if (!rp || !dri || !data->labels)
        return 0;

    pens = dri->dri_Pens;

    x = gad->LeftEdge;
    y = gad->TopEdge;

    itemh = rp->TxHeight + data->spacing;
    if (itemh < RADIO_DIAM + data->spacing)
        itemh = RADIO_DIAM + data->spacing;

    itemw = RADIO_DIAM + 6;

    /* Clear the gadget area */
    SetAPen(rp, pens[BACKGROUNDPEN]);
    RectFill(rp, gad->LeftEdge, gad->TopEdge,
             gad->LeftEdge + gad->Width - 1,
             gad->TopEdge + gad->Height - 1);

    index = 0;
    for (node = data->labels->lh_Head; node->ln_Succ; node = node->ln_Succ)
    {
        /* Vertical layout */
        cx = x + RADIO_RADIUS;
        cy = y + (itemh / 2);

        /* Draw outer circle - shadow */
        SetAPen(rp, pens[SHADOWPEN]);
        DrawEllipse(rp, cx, cy, RADIO_RADIUS, RADIO_RADIUS);

        /* Draw outer circle - shine (offset to give 3D look) */
        SetAPen(rp, pens[SHINEPEN]);
        DrawEllipse(rp, cx, cy, RADIO_RADIUS - 1, RADIO_RADIUS - 1);

        /* Fill selected indicator */
        if (index == data->selected)
        {
            SetAPen(rp, pens[FILLPEN]);
            AreaEllipse(rp, cx, cy, RADIO_RADIUS - 3, RADIO_RADIUS - 3);
        }

        /* Draw the label text */
        if (node->ln_Name)
        {
            WORD tx, ty;

            SetAPen(rp, pens[TEXTPEN]);
            SetBPen(rp, pens[BACKGROUNDPEN]);
            SetDrMd(rp, JAM2);

            tx = x + itemw;
            ty = cy - (rp->TxHeight / 2) + rp->TxBaseline;

            Move(rp, tx, ty);
            Text(rp, node->ln_Name, strlen(node->ln_Name));
        }

        /* Advance to next item position */
        y += itemh;

        index++;
    }

    return 1;
}
