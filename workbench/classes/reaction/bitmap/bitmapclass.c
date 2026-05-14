/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Reaction bitmap.image - BOOPSI class implementation
*/
#define DEBUG 1

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/alib.h>
#include <proto/datatypes.h>

#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <datatypes/datatypes.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <images/bitmap.h>
#include <utility/tagitem.h>

#include <string.h>

#include "bitmap_intern.h"

#define BitmapBase ((struct Library *)(cl->cl_UserData))

/******************************************************************************/

/* Load a picture file via datatypes.library.  Returns a datatypes Object* on
 * success (caller owns it and must DisposeDTObject), or NULL on failure.
 * On success *outBitMap and *outMask are filled (mask may be NULL).  If
 * outWidth/outHeight are non-NULL they receive the source dimensions. */
static APTR bitmap_LoadFile(CONST_STRPTR filename, struct Screen *scr,
                            ULONG precision,
                            struct BitMap **outBitMap, UBYTE **outMask,
                            UWORD *outWidth, UWORD *outHeight)
{
    Object *o;
    struct Task *me = FindTask(NULL);
    struct Process *myproc = NULL;
    APTR oldwindowptr = NULL;

    if (!filename) return NULL;

    /* Only Process tasks have pr_WindowPtr; touching it on a plain Task
     * (e.g. when invoked from an input handler context) would corrupt
     * memory beyond the Task structure. */
    if (me && me->tc_Node.ln_Type == NT_PROCESS)
    {
        myproc = (struct Process *)me;
        oldwindowptr = myproc->pr_WindowPtr;
        myproc->pr_WindowPtr = (APTR)-1;
    }

    o = NewDTObject((APTR)filename,
                    DTA_SourceType,        DTST_FILE,
                    DTA_GroupID,           GID_PICTURE,
                    OBP_Precision,         precision ? precision : PRECISION_IMAGE,
                    PDTA_DestMode,         PMODE_V43,
                    scr ? PDTA_Screen        : TAG_IGNORE, (IPTR)scr,
                    scr ? PDTA_UseFriendBitMap : TAG_IGNORE, TRUE,
                    scr ? PDTA_FreeSourceBitMap : TAG_IGNORE, TRUE,
                    TAG_DONE);

    if (myproc)
        myproc->pr_WindowPtr = oldwindowptr;

    if (o)
    {
        struct FrameInfo fri = {0};
        struct BitMap *bm = NULL;
        UBYTE *mask = NULL;

        DoMethod(o, DTM_FRAMEBOX, (IPTR)NULL, (IPTR)&fri, (IPTR)&fri,
                 sizeof(struct FrameInfo), 0);

        if (fri.fri_Dimensions.Depth > 0 &&
            DoMethod(o, DTM_PROCLAYOUT, (IPTR)NULL, 1))
        {
            struct BitMapHeader *bmhd = NULL;

            GetDTAttrs(o, PDTA_BitMapHeader, (IPTR)&bmhd, TAG_DONE);

            GetDTAttrs(o, PDTA_DestBitMap, (IPTR)&bm, TAG_DONE);
            if (!bm)
                GetDTAttrs(o, PDTA_BitMap, (IPTR)&bm, TAG_DONE);

            GetDTAttrs(o, PDTA_MaskPlane, (IPTR)&mask, TAG_DONE);

            if (bm)
            {
                *outBitMap = bm;
                if (outMask) *outMask = mask;
                if (bmhd)
                {
                    if (outWidth)  *outWidth  = bmhd->bmh_Width;
                    if (outHeight) *outHeight = bmhd->bmh_Height;
                }
                return o;
            }
        }
        DisposeDTObject(o);
    }
    return NULL;
}

/* Load BITMAP_SourceFile / BITMAP_DisabledSourceFile if not yet resolved. */
static void bitmap_ResolveFiles(struct BitmapData *data)
{
    if (data->bd_SourceFile && !data->bd_BitMap && !data->bd_LoadTried)
    {
        struct BitMap *bm = NULL;
        UBYTE *mask = NULL;
        APTR dto = bitmap_LoadFile(data->bd_SourceFile, data->bd_Screen,
                                   data->bd_Precision,
                                   &bm, &mask,
                                   data->bd_Width  ? NULL : &data->bd_Width,
                                   data->bd_Height ? NULL : &data->bd_Height);
        data->bd_LoadTried = TRUE;
        if (dto)
        {
            data->bd_DTObject = dto;
            data->bd_BitMap = bm;
            if (!data->bd_MaskPlane) data->bd_MaskPlane = mask;
        }
    }

    if (data->bd_DisabledSourceFile && !data->bd_DisabledBitMap
        && !data->bd_DisabledLoadTried)
    {
        struct BitMap *bm = NULL;
        UBYTE *mask = NULL;
        APTR dto = bitmap_LoadFile(data->bd_DisabledSourceFile, data->bd_Screen,
                                   data->bd_Precision,
                                   &bm, &mask,
                                   data->bd_DisabledWidth  ? NULL : &data->bd_DisabledWidth,
                                   data->bd_DisabledHeight ? NULL : &data->bd_DisabledHeight);
        data->bd_DisabledLoadTried = TRUE;
        if (dto)
        {
            data->bd_DisabledDTObject = dto;
            data->bd_DisabledBitMap = bm;
            if (!data->bd_DisabledMaskPlane) data->bd_DisabledMaskPlane = mask;
        }
    }
}

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
                if (data->bd_SourceFile != (STRPTR)tag->ti_Data)
                    data->bd_LoadTried = FALSE;
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
            case BITMAP_DisabledBitMap:
                data->bd_DisabledBitMap = (struct BitMap *)tag->ti_Data;
                break;
            case BITMAP_DisabledMaskPlane:
                data->bd_DisabledMaskPlane = (UBYTE *)tag->ti_Data;
                break;
            case BITMAP_DisabledWidth:
                data->bd_DisabledWidth = (UWORD)tag->ti_Data;
                break;
            case BITMAP_DisabledHeight:
                data->bd_DisabledHeight = (UWORD)tag->ti_Data;
                break;
            case BITMAP_DisabledOffsetX:
                data->bd_DisabledOffsetX = (WORD)tag->ti_Data;
                break;
            case BITMAP_DisabledOffsetY:
                data->bd_DisabledOffsetY = (WORD)tag->ti_Data;
                break;
            case BITMAP_DisabledSourceFile:
                if (data->bd_DisabledSourceFile != (STRPTR)tag->ti_Data)
                    data->bd_DisabledLoadTried = FALSE;
                data->bd_DisabledSourceFile = (STRPTR)tag->ti_Data;
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
        D(bug("[BitMap] OM_NEW: obj 0x%p\n", (APTR)retval));
        struct BitmapData *data = INST_DATA(cl, (Object *)retval);

        memset(data, 0, sizeof(struct BitmapData));

        bitmap_set(cl, (Object *)retval, msg);
        bitmap_ResolveFiles(data);
    }

    return retval;
}

/******************************************************************************/

IPTR BitMap__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct BitmapData *data = INST_DATA(cl, o);
    D(bug("[BitMap] OM_DISPOSE: obj 0x%p\n", o));

    if (data->bd_DTObject)
    {
        DisposeDTObject((Object *)data->bd_DTObject);
        data->bd_DTObject = NULL;
        data->bd_BitMap = NULL;
        data->bd_MaskPlane = NULL;
    }
    if (data->bd_DisabledDTObject)
    {
        DisposeDTObject((Object *)data->bd_DisabledDTObject);
        data->bd_DisabledDTObject = NULL;
        data->bd_DisabledBitMap = NULL;
        data->bd_DisabledMaskPlane = NULL;
    }

    return DoSuperMethodA(cl, o, msg);
}

/******************************************************************************/

IPTR BitMap__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct BitmapData *data = INST_DATA(cl, o);
    D(bug("[BitMap] OM_SET: obj 0x%p\n", o));
    IPTR retval = DoSuperMethodA(cl, o, (Msg)msg);
    bitmap_set(cl, o, msg);
    bitmap_ResolveFiles(data);
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

        case BITMAP_OffsetX:
            *msg->opg_Storage = data->bd_OffsetX;
            return TRUE;

        case BITMAP_OffsetY:
            *msg->opg_Storage = data->bd_OffsetY;
            return TRUE;

        case BITMAP_SelectBitMap:
            *msg->opg_Storage = (IPTR)data->bd_SelectBitMap;
            return TRUE;

        case BITMAP_SelectMaskPlane:
            *msg->opg_Storage = (IPTR)data->bd_SelectMaskPlane;
            return TRUE;

        case BITMAP_DisabledBitMap:
            *msg->opg_Storage = (IPTR)data->bd_DisabledBitMap;
            return TRUE;

        case BITMAP_DisabledMaskPlane:
            *msg->opg_Storage = (IPTR)data->bd_DisabledMaskPlane;
            return TRUE;

        case BITMAP_DisabledWidth:
            *msg->opg_Storage = data->bd_DisabledWidth;
            return TRUE;

        case BITMAP_DisabledHeight:
            *msg->opg_Storage = data->bd_DisabledHeight;
            return TRUE;

        case BITMAP_DisabledOffsetX:
            *msg->opg_Storage = data->bd_DisabledOffsetX;
            return TRUE;

        case BITMAP_DisabledOffsetY:
            *msg->opg_Storage = data->bd_DisabledOffsetY;
            return TRUE;

        case BITMAP_DisabledSourceFile:
            *msg->opg_Storage = (IPTR)data->bd_DisabledSourceFile;
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

    D(bug("[BitMap] IM_DRAW: state %d, dimensions %dx%d\n", msg->imp_State, im->Width, im->Height));

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
            if (data->bd_DisabledBitMap)
            {
                bm = data->bd_DisabledBitMap;
                mask = data->bd_DisabledMaskPlane;
                if (data->bd_DisabledWidth)  w = data->bd_DisabledWidth;
                if (data->bd_DisabledHeight) h = data->bd_DisabledHeight;
                srcX = data->bd_DisabledOffsetX;
                srcY = data->bd_DisabledOffsetY;
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

    /* Ghost rendering for disabled state — only when no dedicated
     * disabled bitmap was supplied. */
    if ((msg->imp_State == IDS_DISABLED || msg->imp_State == IDS_INACTIVEDISABLED)
        && !data->bd_DisabledBitMap
        && msg->imp_DrInfo)
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
