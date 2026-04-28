/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction penmap.image - BOOPSI class implementation
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
#include <intuition/screens.h>
#include <images/penmap.h>
#include <utility/tagitem.h>

#include <string.h>

#include "penmap_intern.h"

#define PenMapBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void penmap_set(Class *cl, Object *o, struct opSet *msg)
{
    struct PenMapData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case PENMAP_PenMap:
                data->pd_PenMap = (UBYTE *)tag->ti_Data;
                break;
            case PENMAP_Width:
                data->pd_Width = (UWORD)tag->ti_Data;
                break;
            case PENMAP_Height:
                data->pd_Height = (UWORD)tag->ti_Data;
                break;
            case PENMAP_Screen:
                data->pd_Screen = (struct Screen *)tag->ti_Data;
                break;
            case PENMAP_Precision:
                data->pd_Precision = tag->ti_Data;
                break;
            case PENMAP_SelectPenMap:
                data->pd_SelectPenMap = (UBYTE *)tag->ti_Data;
                break;
            case PENMAP_DisPenMap:
                data->pd_DisPenMap = (UBYTE *)tag->ti_Data;
                break;
            case PENMAP_Transparent:
                data->pd_Transparent = (UBYTE)tag->ti_Data;
                break;
            case PENMAP_NumColors:
                data->pd_NumColors = (UWORD)tag->ti_Data;
                break;
            case PENMAP_RGBData:
                data->pd_RGBData = (ULONG *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

static void penmap_render(struct RastPort *rp, UBYTE *penmap,
    UWORD pmWidth, UWORD pmHeight, WORD x, WORD y,
    WORD destW, WORD destH, UBYTE transparent)
{
    UWORD row, col;
    UBYTE pen;
    UBYTE lastPen = 255;

    if (!penmap || pmWidth == 0 || pmHeight == 0)
        return;

    for (row = 0; row < pmHeight && row < destH; row++)
    {
        for (col = 0; col < pmWidth && col < destW; col++)
        {
            pen = penmap[row * pmWidth + col];

            /* Skip transparent pixels */
            if (pen == transparent)
                continue;

            /* Minimize SetAPen calls */
            if (pen != lastPen)
            {
                SetAPen(rp, pen);
                lastPen = pen;
            }

            WritePixel(rp, x + col, y + row);
        }
    }
}

/******************************************************************************/

IPTR PenMap__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct PenMapData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct PenMapData));
        data->pd_Transparent = 255;

        penmap_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR PenMap__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR PenMap__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    penmap_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR PenMap__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct PenMapData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case PENMAP_PenMap:
            *msg->opg_Storage = (IPTR)data->pd_PenMap;
            return TRUE;

        case PENMAP_Width:
            *msg->opg_Storage = data->pd_Width;
            return TRUE;

        case PENMAP_Height:
            *msg->opg_Storage = data->pd_Height;
            return TRUE;

        case PENMAP_Screen:
            *msg->opg_Storage = (IPTR)data->pd_Screen;
            return TRUE;

        case PENMAP_Precision:
            *msg->opg_Storage = data->pd_Precision;
            return TRUE;

        case PENMAP_SelectPenMap:
            *msg->opg_Storage = (IPTR)data->pd_SelectPenMap;
            return TRUE;

        case PENMAP_DisPenMap:
            *msg->opg_Storage = (IPTR)data->pd_DisPenMap;
            return TRUE;

        case PENMAP_Transparent:
            *msg->opg_Storage = data->pd_Transparent;
            return TRUE;

        case PENMAP_NumColors:
            *msg->opg_Storage = data->pd_NumColors;
            return TRUE;

        case PENMAP_RGBData:
            *msg->opg_Storage = (IPTR)data->pd_RGBData;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR PenMap__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct PenMapData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct DrawInfo *dri = msg->imp_DrInfo;
    UWORD *pens;
    WORD x, y, w, h;
    UBYTE *penmap;

    if (!rp || !dri)
        return FALSE;

    pens = dri->dri_Pens;
    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = im->Width;
    h = im->Height;

    /* Select appropriate penmap based on state */
    switch (msg->imp_State)
    {
        case IDS_SELECTED:
        case IDS_INACTIVESELECTED:
            penmap = data->pd_SelectPenMap ? data->pd_SelectPenMap : data->pd_PenMap;
            break;

        case IDS_DISABLED:
        case IDS_INACTIVEDISABLED:
            penmap = data->pd_DisPenMap ? data->pd_DisPenMap : data->pd_PenMap;
            break;

        default:
            penmap = data->pd_PenMap;
            break;
    }

    if (!penmap)
        return FALSE;

    SetDrMd(rp, JAM1);

    penmap_render(rp, penmap, data->pd_Width, data->pd_Height,
        x, y, w, h, data->pd_Transparent);

    /* Draw disabled ghosting if no specific disabled penmap */
    if ((msg->imp_State == IDS_DISABLED || msg->imp_State == IDS_INACTIVEDISABLED)
        && !data->pd_DisPenMap)
    {
        UWORD ghostPat[] = { 0x2222, 0x8888 };

        SetAPen(rp, pens[BLOCKPEN]);
        SetAfPt(rp, ghostPat, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    return TRUE;
}
