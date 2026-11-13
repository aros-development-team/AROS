/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

/****** ChunkyImage.mui/MUIC_ChunkyImage ************
*
*   NAME
*       MUIC_ChunkyImage
*
*   SUPERCLASS
*       Bitmap.mui
*
*   DESCRIPTION
*       Display an eight-bit indexed image by converting chunky pixel rows into
*       an owned BitMap during Setup. Supply Pixels, NumColors and inherited
*       Bitmap_Width/Bitmap_Height. Palette is optional; omitted palette data
*       leaves color interpretation to Bitmap and any separately supplied source
*       colors or mapping table.
*       The object retains borrowed pixel and palette pointers. Setup reads them
*       again on each setup cycle, allocates an intermediate bitmap and
*       optionally expands the byte palette to ULONG RGB channels. The bitmap
*       depth is the smallest value from one through eight that can hold
*       NumColors entries; even one color uses depth one. Pixel values must be
*       valid indices for the supplied color data.
*       Setup fails for NULL pixels, zero dimensions, a color count outside
*       1..256, or detected local allocation failure. It installs the bitmap and
*       optional expanded colors in Bitmap before calling superclass Setup. On
*       superclass failure it clears those installed pointers and frees the
*       local allocations. Bitmap remapping has its own failure behavior: TRUE
*       is not a guarantee that every remapping allocation succeeded.
*       Cleanup delegates first, then clears the installed bitmap pointer, waits
*       for blits, frees the owned bitmap and expanded palette, and clears their
*       fields. Use the normal Setup/Cleanup lifecycle. Dispose frees a
*       remaining owned bitmap but does not independently free palette32;
*       skipping Cleanup can leak the expanded palette and bypass superclass
*       resource cleanup.
*
*   NOTES
*       The four ChunkyImage setters only store values and delegate their tags.
*       They neither rebuild the bitmap nor request redraw or relayout. Changing
*       source bytes has no automatic display effect. Apply changes outside the
*       active setup interval and let the next Setup perform conversion.
*       Bitmap_Width and Bitmap_Height are also stored locally as UWORD and
*       passed to Bitmap. Avoid negative or out-of-range dimensions: truncation
*       can make conversion dimensions differ from the superclass dimensions.
*       The default local dimensions and color count are zero. Do not overwrite
*       the internal Bitmap_Bitmap or generated SourceColors while they are in
*       use.
*       Without a byte palette, Setup does not clear an independently supplied
*       Bitmap_SourceColors value. An explicit Bitmap_MappingTable follows the
*       superclass mapping path. The other Bitmap restrictions, including its
*       alpha and transparency behavior, still apply; this subclass does not add
*       alpha support or validate all source-buffer bounds.
*
*/
/****** ChunkyImage.mui/MUIA_ChunkyImage_Pixels ************
*
*   NAME
*       MUIA_ChunkyImage_Pixels
*
*   FUNCTION
*       Supply or read the source array of eight-bit palette indices.
*
*   INPUTS
*       Initialize, set and get, borrowed UBYTE pointer. Default NULL; Setup
*       requires a non-NULL pointer.
*
*   NOTES
*       Provide Width bytes per row at offsets separated by Modulo. The readable
*       span must cover at least (Height - 1) * Modulo + Width bytes for
*       positive dimensions, with Modulo at least Width. The class does not
*       check buffer length or each index. Each index must be less than
*       NumColors when using its palette; invalid indices can exceed the
*       generated color table during Bitmap remapping.
*       The object never frees the source. Keep it readable for every Setup that
*       uses it, until replacement or disposal. Get returns the stored pointer
*       with TRUE, not a pointer to the converted bitmap. Set does not rebuild
*       or refresh an already converted image.
*
*/
/****** ChunkyImage.mui/MUIA_ChunkyImage_Palette ************
*
*   NAME
*       MUIA_ChunkyImage_Palette
*
*   FUNCTION
*       Supply or read the optional byte RGB palette for conversion.
*
*   INPUTS
*       Initialize, set and get, borrowed UBYTE pointer. Default NULL.
*
*   NOTES
*       Provide at least NumColors consecutive red/green/blue byte triplets,
*       with no count word or terminator. Setup expands each byte to a 32-bit
*       channel by multiplying by 0x01010101, allocates its own ULONG array and
*       passes that array as Bitmap_SourceColors. Get returns the original byte
*       pointer with TRUE, not the expanded array.
*       Keep source storage readable for future Setup calls. NULL skips
*       expansion and does not clear source colors supplied independently to
*       Bitmap. Set merely stores the pointer; it does not change an existing
*       conversion. Cleanup frees the expanded array, never the caller palette.
*
*/
/****** ChunkyImage.mui/MUIA_ChunkyImage_NumColors ************
*
*   NAME
*       MUIA_ChunkyImage_NumColors
*
*   FUNCTION
*       Specify the number of source palette entries and the conversion depth.
*
*   INPUTS
*       Initialize, set and get; LONG in the header, stored as UWORD. Default
*       zero. Setup requires a stored count from one through 256.
*
*   NOTES
*       There is no setter-time validation: input is converted to the 16-bit
*       field before Setup checks it. Get returns that stored value with TRUE.
*       The bitmap uses at least one plane and enough planes for the count; a
*       non-power-of-two count does not allocate extra palette entries for
*       unused indices. Provide three palette bytes per declared color and keep
*       all pixels within the declared count. Set does not rebuild the image.
*
*/
/****** ChunkyImage.mui/MUIA_ChunkyImage_Modulo ************
*
*   NAME
*       MUIA_ChunkyImage_Modulo
*
*   FUNCTION
*       Specify the source row stride in bytes.
*
*   INPUTS
*       Initialize, set and get; LONG in the header, stored as UWORD. At
*       construction, omitted or stored 0xFFFF is replaced with the final stored
*       Width.
*
*   NOTES
*       Modulo is the distance between row starts, not just the padding after a
*       row. Supply at least Width bytes and enough readable source storage for
*       all rows. Setup passes the stored stride directly to WriteChunkyPixels
*       without validating it.
*       The 0xFFFF sentinel is resolved only once at construction. Later Set of
*       0xFFFF stores 65535 literally, and changing Width does not update
*       Modulo. Get returns the effective stored value with TRUE. Negative or
*       larger inputs truncate to UWORD; Set does not rebuild the image.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_ChunkyImageData
{
    struct BitMap *bm;
    UBYTE *pixels;
    UBYTE *palette;
    ULONG *palette32;
    UWORD width, height, numcolors, modulo;
};

/*
 * Validate stored pixels/dimensions/count, expand optional RGB bytes,
 * allocate owned bitmap and copy chunky rows. Return FALSE for local
 * validation/allocation failure, TRUE after WriteChunkyPixels. Requires
 * readable borrowed source spans and valid indices; no buffer/stride/index
 * checks. Do not call over an existing allocation without Cleanup.
 */
static BOOL make_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);
    struct RastPort temprp;
    WORD depth;

    if (!data->pixels ||
        (data->width < 1) ||
        (data->height < 1) ||
        (data->numcolors < 1) || (data->numcolors > 256))
    {
        return FALSE;
    }

    if (data->palette)
    {
        data->palette32 =
            AllocVec(data->numcolors * 3 * sizeof(ULONG), MEMF_ANY);
        if (!data->palette32)
            return FALSE;

        if (data->palette32)
        {
            UBYTE *src = data->palette;
            ULONG *dest = data->palette32;
            WORD i;

            for (i = 0; i < data->numcolors; i++)
            {
                *dest++ = ((ULONG) (*src++)) * 0x01010101;
                *dest++ = ((ULONG) (*src++)) * 0x01010101;
                *dest++ = ((ULONG) (*src++)) * 0x01010101;
            }
        }
    }

    for (depth = 1; (1L << depth) < data->numcolors; depth++)
    {
    }

    data->bm =
        AllocBitMap(data->width, data->height, depth, BMF_CLEAR, NULL);
    if (!data->bm)
    {
        if (data->palette32)
        {
            FreeVec(data->palette32);
            data->palette32 = NULL;
        }
        return FALSE;
    }

    InitRastPort(&temprp);
    temprp.BitMap = data->bm;

    WriteChunkyPixels(&temprp,
        0,
        0, data->width - 1, data->height - 1, data->pixels, data->modulo);

    return TRUE;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Construct Bitmap, retain borrowed source pointers and UWORD
 * dimensions/count/stride, then resolve constructor stride 0xFFFF to width.
 * Return object or zero on superclass failure; no image conversion or input
 * validation yet. Source targets remain caller-owned.
 */
IPTR ChunkyImage__OM_NEW(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ChunkyImageData *data;
    struct TagItem *tag, *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    data->modulo = 0xFFFF;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_ChunkyImage_Pixels:
            data->pixels = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_Palette:
            data->palette = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_NumColors:
            data->numcolors = (LONG) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_Modulo:
            data->modulo = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Width:
            data->width = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Height:
            data->height = (LONG) tag->ti_Data;
            break;

        }
    }

    if (data->modulo == 0xFFFF)
        data->modulo = data->width;

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Wait for blits and free remaining owned bitmap, then return Bitmap
 * disposal result. Requires normal prior Cleanup; this fallback does not
 * independently free palette32 or clear superclass source pointers. Never
 * free caller source buffers.
 */
IPTR ChunkyImage__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    if (data->bm)
    {
        WaitBlit();
        FreeBitMap(data->bm);
    }

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Store source pointers and UWORD values, including inherited width/height,
 * then return Bitmap setter result. No conversion, redraw, relayout or
 * sentinel resolution. Requires valid caller storage for later Setup; avoid
 * changing dimensions while converted buffers are active.
 */
IPTR ChunkyImage__OM_SET(struct IClass *cl, Object *obj,
    struct opSet *msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_ChunkyImage_Pixels:
            data->pixels = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_Palette:
            data->palette = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_NumColors:
            data->numcolors = (LONG) tag->ti_Data;
            break;

        case MUIA_ChunkyImage_Modulo:
            data->modulo = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Width:
            data->width = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Height:
            data->height = (LONG) tag->ti_Data;
            break;

        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Store original borrowed pixels/palette pointers or effective UWORD
 * count/stride and return TRUE; delegate other getters to Bitmap. Requires
 * writable storage. Returning source data does not certify successful Setup
 * or expose the owned converted buffers.
 */
IPTR ChunkyImage__OM_GET(struct IClass *cl, Object *obj,
    struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_ChunkyImage_Pixels:
        STORE = (IPTR) data->pixels;
        return TRUE;

    case MUIA_ChunkyImage_Palette:
        STORE = (IPTR) data->palette;
        return TRUE;

    case MUIA_ChunkyImage_NumColors:
        STORE = (IPTR) data->numcolors;
        return TRUE;

    case MUIA_ChunkyImage_Modulo:
        STORE = (IPTR) data->modulo;
        return TRUE;

    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Build owned bitmap/palette, install them through setters, then delegate
 * Bitmap Setup. Return FALSE on build or superclass failure, clearing
 * installed pointers and freeing local allocations on the latter path;
 * otherwise TRUE. Requires valid sources and paired Cleanup, with no
 * existing local allocation.
 */
IPTR ChunkyImage__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);

    if (!make_bitmap(cl, obj))
        return FALSE;

    set(obj, MUIA_Bitmap_Bitmap, (IPTR) data->bm);

    if (data->palette32)
    {
        set(obj, MUIA_Bitmap_SourceColors, (IPTR) data->palette32);
    }

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
    {
        set(obj, MUIA_Bitmap_Bitmap, NULL);
        WaitBlit();
        FreeBitMap(data->bm);
        data->bm = NULL;
        if (data->palette32)
        {
            set(obj, MUIA_Bitmap_SourceColors, NULL);

            FreeVec(data->palette32);
            data->palette32 = NULL;
        }
        return FALSE;
    }

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Delegate Bitmap cleanup first, then when bm exists clear bitmap pointer,
 * WaitBlit/free bitmap and clear/free expanded source colors. Return
 * superclass result. Caller sources remain borrowed; generated palette
 * cleanup is inside the non-NULL bitmap branch.
 */
IPTR ChunkyImage__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_ChunkyImageData *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (data->bm)
    {
        set(obj, MUIA_Bitmap_Bitmap, NULL);

        WaitBlit();

        FreeBitMap(data->bm);
        data->bm = NULL;

        if (data->palette32)
        {
            set(obj, MUIA_Bitmap_SourceColors, NULL);
            FreeVec(data->palette32);
            data->palette32 = NULL;
        }
    }

    return retval;
}


/*
 * Dispatch local overrides and return their results; delegate all other
 * messages to the declared superclass. Requires correct method ABI and live
 * object except construction. Borrowed data and owned child/conversion
 * resources follow the class lifecycle contract.
 */
BOOPSI_DISPATCHER(IPTR, ChunkyImage_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return ChunkyImage__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return ChunkyImage__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return ChunkyImage__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return ChunkyImage__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return ChunkyImage__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return ChunkyImage__MUIM_Cleanup(cl, obj,
            (struct MUIP_Cleanup *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_ChunkyImage_desc =
{
    MUIC_ChunkyImage,
    MUIC_Bitmap,
    sizeof(struct MUI_ChunkyImageData),
    (void *) ChunkyImage_Dispatcher
};
