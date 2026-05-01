/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getscreenmode.gadget - BOOPSI class implementation
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
#include <intuition/cghooks.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <gadgets/getscreenmode.h>
#include <utility/tagitem.h>

#include <string.h>
#include <stdio.h>

#include "getscreenmode_intern.h"

#define GetScreenModeBase ((struct Library *)(cl->cl_UserData))

#define BROWSE_BUTTON_WIDTH 24

/******************************************************************************/

static void getscreenmode_set(Class *cl, Object *o, struct opSet *msg)
{
    struct GetScreenModeData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case GETSCREENMODE_TitleText:
                data->titletext = (STRPTR)tag->ti_Data;
                break;
            case GETSCREENMODE_DisplayID:
                data->displayid = (ULONG)tag->ti_Data;
                break;
            case GETSCREENMODE_DisplayWidth:
                data->displaywidth = (ULONG)tag->ti_Data;
                break;
            case GETSCREENMODE_DisplayHeight:
                data->displayheight = (ULONG)tag->ti_Data;
                break;
            case GETSCREENMODE_DisplayDepth:
                data->displaydepth = (ULONG)tag->ti_Data;
                break;
            case GETSCREENMODE_OverscanType:
                data->overscantype = (ULONG)tag->ti_Data;
                break;
            case GETSCREENMODE_AutoScroll:
                data->autoscroll = (BOOL)tag->ti_Data;
                break;
            case GETSCREENMODE_DoWidth:
                data->dowidth = (BOOL)tag->ti_Data;
                break;
            case GETSCREENMODE_DoHeight:
                data->doheight = (BOOL)tag->ti_Data;
                break;
            case GETSCREENMODE_DoDepth:
                data->dodepth = (BOOL)tag->ti_Data;
                break;
            case GETSCREENMODE_DoOverscanType:
                data->dooverscan = (BOOL)tag->ti_Data;
                break;
            case GETSCREENMODE_DoAutoScroll:
                data->doautoscroll = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR GetScreenMode__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct GetScreenModeData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct GetScreenModeData));

        /* Set defaults */
        data->displayid    = 0xFFFFFFFF; /* INVALID_ID */
        data->dowidth      = TRUE;
        data->doheight     = TRUE;
        data->dodepth      = TRUE;

        getscreenmode_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR GetScreenMode__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR GetScreenMode__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    getscreenmode_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR GetScreenMode__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct GetScreenModeData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case GETSCREENMODE_TitleText:
            *msg->opg_Storage = (IPTR)data->titletext;
            return TRUE;

        case GETSCREENMODE_DisplayID:
            *msg->opg_Storage = data->displayid;
            return TRUE;

        case GETSCREENMODE_DisplayWidth:
            *msg->opg_Storage = data->displaywidth;
            return TRUE;

        case GETSCREENMODE_DisplayHeight:
            *msg->opg_Storage = data->displayheight;
            return TRUE;

        case GETSCREENMODE_DisplayDepth:
            *msg->opg_Storage = data->displaydepth;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR GetScreenMode__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct GetScreenModeData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    UWORD bgpen, textpen, shinepen, shadowpen;
    char modebuf[64];

    if (!rp && msg->gpr_GInfo)
        rp = ObtainGIRPort(msg->gpr_GInfo);

    if (!rp)
        return FALSE;

    x = gad->LeftEdge;
    y = gad->TopEdge;
    w = gad->Width;
    h = gad->Height;

    bgpen     = dri ? dri->dri_Pens[BACKGROUNDPEN] : 0;
    textpen   = dri ? dri->dri_Pens[TEXTPEN] : 1;
    shinepen  = dri ? dri->dri_Pens[SHINEPEN] : 2;
    shadowpen = dri ? dri->dri_Pens[SHADOWPEN] : 1;

    /* Fill background */
    SetAPen(rp, bgpen);
    RectFill(rp, x, y, x + w - 1, y + h - 1);

    /* Draw recessed frame for the text field area */
    SetAPen(rp, shadowpen);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + w - BROWSE_BUTTON_WIDTH - 1, y);

    SetAPen(rp, shinepen);
    Move(rp, x + 1, y + h - 1);
    Draw(rp, x + w - BROWSE_BUTTON_WIDTH - 1, y + h - 1);
    Draw(rp, x + w - BROWSE_BUTTON_WIDTH - 1, y + 1);

    /* Build mode info string */
    if (data->displayid != 0xFFFFFFFF)
    {
        snprintf(modebuf, sizeof(modebuf), "0x%08lX %lux%lux%lu",
                 (unsigned long)data->displayid,
                 (unsigned long)data->displaywidth,
                 (unsigned long)data->displayheight,
                 (unsigned long)data->displaydepth);
    }
    else
    {
        strncpy(modebuf, "(no mode selected)", sizeof(modebuf) - 1);
        modebuf[sizeof(modebuf) - 1] = '\0';
    }

    /* Draw mode info text */
    SetAPen(rp, textpen);
    SetDrMd(rp, JAM1);
    Move(rp, x + 4, y + rp->TxBaseline + (h - rp->TxHeight) / 2);
    Text(rp, modebuf, strlen(modebuf));

    /* Draw browse button "..." */
    {
        WORD bx = x + w - BROWSE_BUTTON_WIDTH;
        WORD by = y;
        WORD bw = BROWSE_BUTTON_WIDTH;
        WORD bh = h;

        /* Raised button frame */
        SetAPen(rp, shinepen);
        Move(rp, bx, by + bh - 1);
        Draw(rp, bx, by);
        Draw(rp, bx + bw - 1, by);

        SetAPen(rp, shadowpen);
        Move(rp, bx + 1, by + bh - 1);
        Draw(rp, bx + bw - 1, by + bh - 1);
        Draw(rp, bx + bw - 1, by + 1);

        /* Fill button interior */
        SetAPen(rp, bgpen);
        RectFill(rp, bx + 1, by + 1, bx + bw - 2, by + bh - 2);

        /* Draw "..." label centered */
        SetAPen(rp, textpen);
        SetDrMd(rp, JAM1);
        Move(rp, bx + (bw - rp->TxWidth * 3) / 2,
             by + rp->TxBaseline + (bh - rp->TxHeight) / 2);
        Text(rp, "...", 3);
    }

    /* Disabled rendering */
    if (gad->Flags & GFLG_DISABLED)
    {
        ULONG pattern[] = { 0x88888888, 0x22222222 };
        SetAPen(rp, bgpen);
        SetAfPt(rp, (UWORD *)pattern, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    if (!msg->gpr_RPort && msg->gpr_GInfo)
        ReleaseGIRPort(rp);

    return TRUE;
}
