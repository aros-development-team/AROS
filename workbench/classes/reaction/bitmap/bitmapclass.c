/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bitmap.image - BOOPSI class implementation
*/

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <images/bitmap.h>
#include <utility/tagitem.h>

#include <string.h>

#include "bitmap_intern.h"

#define BitmapBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

static void bitmap_set(Class *cl, Object *o, struct opSet *msg)
{
    struct BitmapData *data = INST_DATA(cl, o);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)))
    {
        switch (tag->ti_Tag)
        {
            case BITMAP_BitMap:
                data->bd_BitMap = (struct BitMap *)tag->ti_Data;
                break;
            case BITMAP_MaskPlane:
                data->bd_MaskPlane = (UBYTE *)tag->ti_Data;
                break;
            case BITMAP_Width:
                data->bd_Width = (UWORD)tag->ti_Data;
                break;
            case BITMAP_Height:
                data->bd_Height = (UWORD)tag->ti_Data;
                break;
            case BITMAP_SourceFile:
                data->bd_SourceFile = (STRPTR)tag->ti_Data;
                break;
            case BITMAP_Screen:
                data->bd_Screen = (struct Screen *)tag->ti_Data;
                break;
            case BITMAP_Precision:
                data->bd_Precision = tag->ti_Data;
                break;
            case BITMAP_Masking:
                data->bd_Masking = (BOOL)tag->ti_Data;
                break;
            case BITMAP_Transparent:
                data->bd_Transparent = (BOOL)tag->ti_Data;
                break;
            case BITMAP_OffsetX:
                data->bd_OffsetX = (WORD)tag->ti_Data;
                break;
            case BITMAP_OffsetY:
                data->bd_OffsetY = (WORD)tag->ti_Data;
                break;
            case BITMAP_SelectBitMap:
                data->bd_SelectBitMap = (struct BitMap *)tag->ti_Data;
                break;
            case BITMAP_SelectMaskPlane:
                data->bd_SelectMaskPlane = (UBYTE *)tag->ti_Data;
                break;
            case BITMAP_DisBitMap:
                data->bd_DisBitMap = (struct BitMap *)tag->ti_Data;
                break;
            case BITMAP_DisMaskPlane:
                data->bd_DisMaskPlane = (UBYTE *)tag->ti_Data;
                break;
        }
    }
}

/******************************************************************************/

IPTR BitMap__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval;

    retval = DoSuperMethodA(cl, o, (Msg)msg);
    if (retval)
    {
        struct BitmapData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct BitmapData));

        bitmap_set(cl, (Object *)retval, msg);
    }

    return retval;
}

/******************************************************************************/

IPTR BitMap__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR BitMap__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    bitmap_set(cl, o, msg);
    return retval;
}

/******************************************************************************/

IPTR BitMap__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct BitmapData *data = INST_DATA(cl, o);

    switch (msg->opg_AttrID)
    {
        case BITMAP_BitMap:
            *msg->opg_Storage = (IPTR)data->bd_BitMap;
            return TRUE;

        case BITMAP_MaskPlane:
            *msg->opg_Storage = (IPTR)data->bd_MaskPlane;
            return TRUE;

        case BITMAP_Width:
            *msg->opg_Storage = data->bd_Width;
            return TRUE;

        case BITMAP_Height:
            *msg->opg_Storage = data->bd_Height;
            return TRUE;

        case BITMAP_SourceFile:
            *msg->opg_Storage = (IPTR)data->bd_SourceFile;
            return TRUE;

        case BITMAP_Screen:
            *msg->opg_Storage = (IPTR)data->bd_Screen;
            return TRUE;

        case BITMAP_Precision:
            *msg->opg_Storage = data->bd_Precision;
            return TRUE;

        case BITMAP_Masking:
            *msg->opg_Storage = data->bd_Masking;
            return TRUE;

        case BITMAP_Transparent:
            *msg->opg_Storage = data->bd_Transparent;
            return TRUE;

        case BITMAP_OffsetX:
            *msg->opg_Storage = data->bd_OffsetX;
            return TRUE;

        case BITMAP_OffsetY:
            *msg->opg_Storage = data->bd_OffsetY;
            return TRUE;

        case BITMAP_SelectBitMap:
            *msg->opg_Storage = (IPTR)data->bd_SelectBitMap;
            return TRUE;

        case BITMAP_DisBitMap:
            *msg->opg_Storage = (IPTR)data->bd_DisBitMap;
            return TRUE;
    }

    return DoSuperMethodA(cl, o, (Msg)msg);
}

/******************************************************************************/

IPTR BitMap__IM_DRAW(Class *cl, Object *o, struct impDraw *msg)
{
    struct BitmapData *data = INST_DATA(cl, o);
    struct Image *im = (struct Image *)o;
    struct RastPort *rp = msg->imp_RPort;
    struct BitMap *bm = NULL;
    UBYTE *mask = NULL;
    WORD x, y, w, h;
    WORD srcX, srcY;

    if (!rp)
        return FALSE;

    x = im->LeftEdge + msg->imp_Offset.X;
    y = im->TopEdge + msg->imp_Offset.Y;
    w = data->bd_Width ? data->bd_Width : im->Width;
    h = data->bd_Height ? data->bd_Height : im->Height;
    srcX = data->bd_OffsetX;
    srcY = data->bd_OffsetY;

    /* Select bitmap based on draw state */
    switch (msg->imp_State)
    {
        case IDS_SELECTED:
        case IDS_INACTIVESELECTED:
            if (data->bd_SelectBitMap)
            {
                bm = data->bd_SelectBitMap;
                mask = data->bd_SelectMaskPlane;
            }
            else
            {
                bm = data->bd_BitMap;
                mask = data->bd_MaskPlane;
            }
            break;

        case IDS_DISABLED:
        case IDS_INACTIVEDISABLED:
            if (data->bd_DisBitMap)
            {
                bm = data->bd_DisBitMap;
                mask = data->bd_DisMaskPlane;
            }
            else
            {
                bm = data->bd_BitMap;
                mask = data->bd_MaskPlane;
            }
            break;

        case IDS_NORMAL:
        case IDS_INACTIVENORMAL:
        default:
            bm = data->bd_BitMap;
            mask = data->bd_MaskPlane;
            break;
    }

    if (!bm)
        return TRUE;

    if (data->bd_Masking && mask)
    {
        BltMaskBitMapRastPort(bm, srcX, srcY, rp, x, y, w, h,
            0xC0, mask);
    }
    else
    {
        BltBitMapRastPort(bm, srcX, srcY, rp, x, y, w, h, 0xC0);
    }

    /* Ghost rendering for disabled state if no disabled bitmap */
    if ((msg->imp_State == IDS_DISABLED || msg->imp_State == IDS_INACTIVEDISABLED)
        && !data->bd_DisBitMap && msg->imp_DrInfo)
    {
        UWORD ghostPat[] = { 0x4444, 0x1111 };

        SetAPen(rp, msg->imp_DrInfo->dri_Pens[BACKGROUNDPEN]);
        SetDrMd(rp, JAM1);
        SetAfPt(rp, ghostPat, 1);
        RectFill(rp, x, y, x + w - 1, y + h - 1);
        SetAfPt(rp, NULL, 0);
    }

    return TRUE;
}
