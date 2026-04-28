/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction getfile.gadget - BOOPSI class implementation
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
#include <gadgets/getfile.h>
#include <utility/tagitem.h>

#include <string.h>

#include "getfile_intern.h"

#define GetFileBase ((struct Library *)(cl->cl_UserData))

#define BROWSE_BUTTON_WIDTH 24

/******************************************************************************/

static void getfile_set(Class *cl, Object *o, struct opSet *msg)
{
    struct GetFileData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case GETFILE_TitleText:
                data->gfd_TitleText = (STRPTR)tag->ti_Data;
                break;
            case GETFILE_File:
                data->gfd_File = (STRPTR)tag->ti_Data;
                break;
            case GETFILE_Drawer:
                data->gfd_Drawer = (STRPTR)tag->ti_Data;
                break;
            case GETFILE_Pattern:
                data->gfd_Pattern = (STRPTR)tag->ti_Data;
                break;
            case GETFILE_DoSaveMode:
                data->gfd_DoSaveMode = (BOOL)tag->ti_Data;
                break;
            case GETFILE_DoMultiSelect:
                data->gfd_DoMultiSelect = (BOOL)tag->ti_Data;
                break;
            case GETFILE_DoPatterns:
                data->gfd_DoPatterns = (BOOL)tag->ti_Data;
                break;
            case GETFILE_RejectIcons:
                data->gfd_RejectIcons = (BOOL)tag->ti_Data;
                break;
            case GETFILE_FullFile:
                data->gfd_FullFile = (STRPTR)tag->ti_Data;
                break;
            case GETFILE_ReadOnly:
                data->gfd_ReadOnly = (BOOL)tag->ti_Data;
                break;
            case GETFILE_DrawersOnly:
                data->gfd_DrawersOnly = (BOOL)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR GetFile__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct GetFileData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct GetFileData));

        /* Default title text */
        data->gfd_TitleText = "Select File";

        getfile_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR GetFile__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR GetFile__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    getfile_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR GetFile__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct GetFileData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case GETFILE_TitleText:
            *msg->opg_Storage = (IPTR)data->gfd_TitleText;
            return TRUE;

        case GETFILE_File:
            *msg->opg_Storage = (IPTR)data->gfd_File;
            return TRUE;

        case GETFILE_Drawer:
            *msg->opg_Storage = (IPTR)data->gfd_Drawer;
            return TRUE;

        case GETFILE_Pattern:
            *msg->opg_Storage = (IPTR)data->gfd_Pattern;
            return TRUE;

        case GETFILE_FullFile:
            *msg->opg_Storage = (IPTR)data->gfd_FullFile;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR GetFile__GM_RENDER(Class *cl, Object *o, struct gpRender *msg)
{
    struct GetFileData *data = INST_DATA(cl, o);
    struct RastPort *rp = msg->gpr_RPort;
    struct DrawInfo *dri = msg->gpr_GInfo ? msg->gpr_GInfo->gi_DrInfo : NULL;
    struct Gadget *gad = G(o);
    WORD x, y, w, h;
    WORD tx, ty, tw;
    WORD bx;
    UWORD bgpen, textpen, shinepen, shadowpen;
    STRPTR disptext;

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

    /* Draw file path text */
    disptext = data->gfd_FullFile ? data->gfd_FullFile : data->gfd_File;
    if (disptext)
    {
        WORD len = strlen(disptext);
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
        Text(rp, disptext, len);
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
