/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction radiobutton.gadget - BOOPSI class implementation
*/
#define DEBUG 1

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
#include <reaction/reaction_prefs.h>
#include <exec/semaphores.h>
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

            case RADIOBUTTON_LabelArray:
            {
                STRPTR *arr = (STRPTR *)tag->ti_Data;

                /* Free any previously auto-built list */
                if (data->rd_AutoListUsed)
                {
                    struct Node *n;
                    while ((n = RemHead(&data->rd_AutoList)) != NULL)
                        FreeVec(n);
                }

                NEWLIST(&data->rd_AutoList);
                data->rd_AutoListUsed = TRUE;

                if (arr)
                {
                    while (*arr)
                    {
                        struct Node *n = (struct Node *)AllocVec(sizeof(struct Node), MEMF_CLEAR);
                        if (!n) break;
                        n->ln_Name = (STRPTR)*arr;
                        AddTail(&data->rd_AutoList, n);
                        arr++;
                    }
                }
                data->labels = &data->rd_AutoList;
                break;
            }

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

    D(bug("[RadioButton] OM_NEW: entry\n"));
    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        D(bug("[RadioButton] OM_NEW: obj=%p\n", (Object *)retval));
        struct RadioButtonData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct RadioButtonData));

        /* Set defaults */
        data->selected    = 0;
        data->spacing     = 4;

        /* Snapshot prefs */
        {
            struct UIPrefs *prefs;
            prefs = (struct UIPrefs *)FindSemaphore((STRPTR)RAPREFSSEMAPHORE);
            if (prefs)
            {
                ObtainSemaphoreShared(&prefs->cap_Semaphore);
                data->rd_PrefsLabelPen = prefs->cap_LabelPen;
                data->rd_3DLabel       = prefs->cap_3DLabel ? TRUE : FALSE;
                ReleaseSemaphore(&prefs->cap_Semaphore);
            }
        }

        radiobutton_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/* ====================================================================== */

IPTR RadioButton__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct RadioButtonData *data = INST_DATA(cl, o);

    D(bug("[RadioButton] OM_DISPOSE: entry\n"));

    /* Free any auto-built label list (caller-owned RADIOBUTTON_Labels lists
     * are NOT freed). */
    if (data->rd_AutoListUsed)
    {
        struct Node *n;
        while ((n = RemHead(&data->rd_AutoList)) != NULL)
            FreeVec(n);
        data->rd_AutoListUsed = FALSE;
    }

    return DoSuperMethodA(cl, o, msg);
}

/* ====================================================================== */

IPTR RadioButton__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    D(bug("[RadioButton] OM_SET: entry\n"));
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
    D(bug("[RadioButton] GM_RENDER: redraw=%d\n", msg->gpr_Redraw));
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

        /* Fill selected indicator (small disk, drawn without AreaInfo
         * so we don't depend on RastPort having a valid AreaInfo/TmpRas). */
        if (index == data->selected)
        {
            WORD ir = RADIO_RADIUS - 3;
            if (ir < 1) ir = 1;
            SetAPen(rp, pens[FILLPEN]);
            RectFill(rp, cx - ir, cy - ir, cx + ir, cy + ir);
        }

        /* Draw the label text */
        if (node->ln_Name)
        {
            WORD tx, ty;
            UWORD textPen = data->rd_PrefsLabelPen
                          ? data->rd_PrefsLabelPen
                          : pens[TEXTPEN];

            SetBPen(rp, pens[BACKGROUNDPEN]);
            SetDrMd(rp, JAM1);

            tx = x + itemw;
            ty = cy - (rp->TxHeight / 2) + rp->TxBaseline;

            if (data->rd_3DLabel)
            {
                SetAPen(rp, pens[SHINEPEN]);
                Move(rp, tx + 1, ty + 1);
                Text(rp, node->ln_Name, strlen(node->ln_Name));
            }
            SetAPen(rp, textPen);
            Move(rp, tx, ty);
            Text(rp, node->ln_Name, strlen(node->ln_Name));
        }

        /* Advance to next item position */
        y += itemh;

        index++;
    }

    return 1;
}
