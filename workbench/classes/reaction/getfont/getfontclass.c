/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfont.gadget - BOOPSI class implementation
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
#include <graphics/gfxmacros.h>
#include <graphics/text.h>
#include <gadgets/getfont.h>
#include <utility/tagitem.h>

#include <string.h>
#include <stdio.h>

#include "getfont_intern.h"

#define GetFontBase ((struct Library *)(cl->cl_UserData))

#define BROWSE_BUTTON_WIDTH 24

/******************************************************************************/

static void getfont_set(Class *cl, Object *o, struct opSet *msg)
{
    struct GetFontData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case GETFONT_TitleText:
                data->gfd_TitleText = (STRPTR)tag->ti_Data;
                break;
            case GETFONT_TextAttr:
                data->gfd_TextAttr = (struct TextAttr *)tag->ti_Data;
                break;
            case GETFONT_FontName:
                data->gfd_FontName = (STRPTR)tag->ti_Data;
                break;
            case GETFONT_FontSize:
                data->gfd_FontSize = (UWORD)tag->ti_Data;
                break;
            case GETFONT_FontStyle:
                data->gfd_FontStyle = (UWORD)tag->ti_Data;
                break;
            case GETFONT_DoStyle:
                data->gfd_DoStyle = (BOOL)tag->ti_Data;
                break;
            case GETFONT_FixedWidthOnly:
                data->gfd_FixedWidthOnly = (BOOL)tag->ti_Data;
                break;
            case GETFONT_MinHeight:
                data->gfd_MinHeight = (UWORD)tag->ti_Data;
                break;
            case GETFONT_MaxHeight:
                data->gfd_MaxHeight = (UWORD)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR GetFont__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct GetFontData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct GetFontData));

        /* Default values */
        data->gfd_TitleText = "Select Font";
        data->gfd_FontSize = 8;

        getfont_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR GetFont__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR GetFont__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    getfont_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR GetFont__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct GetFontData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case GETFONT_TitleText:
            *msg->opg_Storage = (IPTR)data->gfd_TitleText;
            return TRUE;

        case GETFONT_TextAttr:
            *msg->opg_Storage = (IPTR)data->gfd_TextAttr;
            return TRUE;

        case GETFONT_FontName:
            *msg->opg_Storage = (IPTR)data->gfd_FontName;
            return TRUE;

        case GETFONT_FontSize:
            *msg->opg_Storage = (IPTR)data->gfd_FontSize;
            return TRUE;

        case GETFONT_FontStyle:
            *msg->opg_Storage = (IPTR)data->gfd_FontStyle;
            return TRUE;

        case GETFONT_DoStyle:
            *msg->opg_Storage = (IPTR)data->gfd_DoStyle;
            return TRUE;

        case GETFONT_FixedWidthOnly:
            *msg->opg_Storage = (IPTR)data->gfd_FixedWidthOnly;
            return TRUE;

        case GETFONT_MinHeight:
            *msg->opg_Storage = (IPTR)data->gfd_MinHeight;
            return TRUE;

        case GETFONT_MaxHeight:
            *msg->opg_Storage = (IPTR)data->gfd_MaxHeight;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR GetFont__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct GetFontData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    WORD tx, ty, tw;
    WORD bx;
    UWORD bgpen, textpen, shinepen, shadowpen;
    char dispbuf[128];

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

    /* Text field area (left portion) */
    tw = w - BROWSE_BUTTON_WIDTH - 2;
    if (tw < 0) tw = 0;

    /* Draw recessed text field frame */
    SetAPen(rp, shadowpen);
    Move(rp, x, y + h - 1);
    Draw(rp, x, y);
    Draw(rp, x + tw - 1, y);

    SetAPen(rp, shinepen);
    Move(rp, x + 1, y + h - 1);
    Draw(rp, x + tw - 1, y + h - 1);
    Draw(rp, x + tw - 1, y + 1);

    /* Fill text field interior */
    SetAPen(rp, bgpen);
    RectFill(rp, x + 1, y + 1, x + tw - 2, y + h - 2);

    /* Build display text showing font name and size */
    if (data->gfd_FontName)
    {
        snprintf(dispbuf, sizeof(dispbuf), "%s/%d",
                 data->gfd_FontName, (int)data->gfd_FontSize);
    }
    else
    {
        snprintf(dispbuf, sizeof(dispbuf), "%d", (int)data->gfd_FontSize);
    }

    /* Draw font info text */
    {
        WORD len = strlen(dispbuf);
        WORD maxchars;

        SetAPen(rp, textpen);
        SetDrMd(rp, JAM1);

        tx = x + 4;
        ty = y + 1 + rp->TxBaseline;

        /* Clip text to field width */
        maxchars = (tw - 8) / rp->TxWidth;
        if (maxchars < 0) maxchars = 0;
        if (len > maxchars) len = maxchars;

        Move(rp, tx, ty);
        Text(rp, dispbuf, len);
    }

    /* Draw browse button "..." */
    bx = x + tw + 2;

    /* Button raised frame */
    SetAPen(rp, shinepen);
    Move(rp, bx, y + h - 1);
    Draw(rp, bx, y);
    Draw(rp, bx + BROWSE_BUTTON_WIDTH - 1, y);

    SetAPen(rp, shadowpen);
    Move(rp, bx + 1, y + h - 1);
    Draw(rp, bx + BROWSE_BUTTON_WIDTH - 1, y + h - 1);
    Draw(rp, bx + BROWSE_BUTTON_WIDTH - 1, y + 1);

    /* Fill button interior */
    SetAPen(rp, bgpen);
    RectFill(rp, bx + 1, y + 1, bx + BROWSE_BUTTON_WIDTH - 2, y + h - 2);

    /* Draw "..." label centered in the button */
    SetAPen(rp, textpen);
    SetDrMd(rp, JAM1);
    tx = bx + (BROWSE_BUTTON_WIDTH - rp->TxWidth * 3) / 2;
    ty = y + 1 + rp->TxBaseline;
    Move(rp, tx, ty);
    Text(rp, "...", 3);

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
