/*
    Copyright (C) 2002-2026, The AROS Development Team. All rights reserved.
*/

#include <graphics/gfx.h>
#include <graphics/view.h>
#include <clib/alib_protos.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>
#include <proto/intuition.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"

#include <string.h>

/****** Bitmap.mui/MUIC_Bitmap ************
*
*   NAME
*       MUIC_Bitmap
*
*   SUPERCLASS
*       Area.mui
*
*   DESCRIPTION
*       Display a caller-owned bitmap at the upper-left content position,
*       cropping to the smaller of configured dimensions and available content
*       dimensions. No scaling is performed. Width and Height default to zero
*       and are not inferred from the source bitmap. Source bitmap, MappingTable
*       and SourceColors pointers are borrowed; keep them valid through their
*       use and later setup cycles.
*       Setup calls Area then optionally remaps an indexed source using
*       MappingTable or SourceColors. Remapping requires a source depth at most
*       eight, positive configured dimensions and at least one of those tables.
*       MappingTable takes precedence. Setup returns TRUE after Area success
*       even if remapping allocations fail. Draw chooses the remapped bitmap
*       when present, otherwise the original source.
*
*   METHODS
*       AskMinMax adds one pixel to superclass minimum/default dimensions and
*       sets both maxima to MUI_MAXMAX; configured bitmap dimensions do not
*       determine these requirements. It returns TRUE. Draw calls Area and
*       returns TRUE even with no image or invalid dimensions. There is no local
*       drawing-flag test. If Alpha has ever been supplied, the local
*       image-drawing branch is unimplemented and draws nothing.
*       Set stores supported attributes but requests no remapping, redraw or
*       relayout. Bitmap replacement is ignored while a remapped bitmap exists.
*       Cleanup frees the transparency raster, waits for blits before freeing
*       the remapped bitmap, and releases cached pens through the live screen
*       ColorMap. Dispose additionally frees the remap table, but does not
*       substitute for Cleanup's raster and pen release. Follow the normal
*       setup/cleanup lifecycle.
*
*   BUGS
*       Transparency-raster allocation is followed by memset without checking
*       for NULL. Remapping uses WORD dimensions and loop counters and unchecked
*       allocation arithmetic; source coverage and table lengths are not
*       validated. Keep configured dimensions within both bitmap bounds and the
*       representable implementation range.
*       In the SourceColors pen path, an obtained pen plus a 0x100 marker is
*       assigned through a UBYTE temporary before storage in the WORD cache. The
*       marker is lost; pen zero is repeatedly obtained and not recognized for
*       release. Pen acquisition failure is also unchecked. Successful Setup
*       therefore does not establish successful remapping or balanced pen
*       acquisition for every palette.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Alpha ************
*
*   NAME
*       MUIA_Bitmap_Alpha
*
*   FUNCTION
*       Store the requested alpha value and select the alpha drawing path.
*
*   INPUTS
*       Initialize, set and get, ULONG. Stored default 0xffffffff; alpha mode
*       starts disabled.
*
*   NOTES
*       Supplying this tag, even with the default value, permanently enables the
*       local alpha flag. Its drawing branch is a TODO and performs no image
*       blit. No tag disables that flag. Get returns the stored value, not
*       whether alpha mode is enabled. Set requests no redraw; background/frame
*       drawing remains inherited.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Bitmap ************
*
*   NAME
*       MUIA_Bitmap_Bitmap
*
*   FUNCTION
*       Supply or retrieve the source bitmap.
*
*   INPUTS
*       Initialize, set and get, struct BitMap pointer. Default NULL.
*
*   NOTES
*       The pointer is borrowed and is never freed by Bitmap. Configured
*       Width/Height must describe accessible source pixels. A setter is ignored
*       while remapped storage exists, although its tag is still forwarded to
*       Area. No remap or redraw is requested. Get returns the source pointer,
*       not the remapped image; NULL means no source image.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Width ************
*
*   NAME
*       MUIA_Bitmap_Width
*
*   FUNCTION
*       Set or retrieve the configured image width.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero.
*
*   NOTES
*       Use a positive dimension covered by the source bitmap. Draw crops to the
*       available content dimension and skips the blit if either resulting
*       dimension is nonpositive. Dimensions do not set minimum/default object
*       size, which remains one content pixel. Set does not resize existing
*       remapped storage, remap, redraw or relayout; changing dimensions while
*       set up can make them inconsistent with allocated resources. Remapping
*       uses narrower WORD counters.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Height ************
*
*   NAME
*       MUIA_Bitmap_Height
*
*   FUNCTION
*       Set or retrieve the configured image height.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero.
*
*   NOTES
*       Use a positive dimension covered by the source bitmap. Draw crops to the
*       available content dimension and skips the blit if either resulting
*       dimension is nonpositive. Dimensions do not set minimum/default object
*       size, which remains one content pixel. Set does not resize existing
*       remapped storage, remap, redraw or relayout; changing dimensions while
*       set up can make them inconsistent with allocated resources. Remapping
*       uses narrower WORD counters.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_MappingTable ************
*
*   NAME
*       MUIA_Bitmap_MappingTable
*
*   FUNCTION
*       Provide a source-index to destination-pen mapping.
*
*   INPUTS
*       Initialize, set and get, UBYTE pointer. Default NULL.
*
*   NOTES
*       The table is borrowed; provide a readable entry for every source pixel
*       index and keep it live during remapping. A 256-entry table covers all
*       byte indices. This table takes precedence over SourceColors and bypasses
*       pen acquisition. The caller manages destination pens and their lifetime.
*       Get returns the same borrowed pointer. Set does not rebuild existing
*       remapped data.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_SourceColors ************
*
*   NAME
*       MUIA_Bitmap_SourceColors
*
*   FUNCTION
*       Provide RGB components for indexed-source remapping.
*
*   INPUTS
*       Initialize, set and get, ULONG pointer. Default NULL.
*
*   NOTES
*       The borrowed table contains three ULONG components per source index, in
*       red/green/blue order, in the representation expected by ObtainBestPenA.
*       Provide all referenced entries; the class checks no length. MappingTable
*       takes precedence. Setup may obtain screen pens and cache them for
*       Cleanup; pen-zero caching and failure handling have the limitations
*       described in the class entry. Get returns the borrowed pointer. Set does
*       not remap or redraw.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Precision ************
*
*   NAME
*       MUIA_Bitmap_Precision
*
*   FUNCTION
*       Select pen matching precision for SourceColors remapping.
*
*   INPUTS
*       Initialize, set and get, LONG. Default PRECISION_GUI.
*
*   NOTES
*       Passed as OBP_Precision to ObtainBestPenA, together with OBP_FailIfBad
*       FALSE. Not used by the explicit MappingTable path. No value validation
*       or live remapping occurs; Set only changes stored state for later use.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_RemappedBitmap ************
*
*   NAME
*       MUIA_Bitmap_RemappedBitmap
*
*   FUNCTION
*       Retrieve the internally allocated remapped bitmap.
*
*   INPUTS
*       Get only, struct BitMap pointer. Default NULL.
*
*   NOTES
*       Returns class-owned storage without transferring ownership. It can be
*       NULL because remapping was unnecessary, unsupported or failed; Setup
*       success does not distinguish these outcomes. Do not free it or retain it
*       after Cleanup/disposal. The getter does not fall back to the original
*       source bitmap.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_Transparent ************
*
*   NAME
*       MUIA_Bitmap_Transparent
*
*   FUNCTION
*       Choose the source palette index excluded by a generated mask.
*
*   INPUTS
*       Initialize, set and get, LONG. Default -1 disables mask generation.
*
*   NOTES
*       A value other than -1 requests a mask during indexed remapping. Matching
*       is against original byte indices before remapping. Without a remapping
*       pass, no mask is generated and this value alone does not make pixels
*       transparent. Set does not regenerate or remove an existing mask. Mask
*       allocation failure is not checked before initialization. This is
*       palette-index transparency, not the Alpha path.
*
*/
/****** Bitmap.mui/MUIA_Bitmap_UseFriend ************
*
*   NAME
*       MUIA_Bitmap_UseFriend
*
*   FUNCTION
*       Request the screen bitmap as friend when allocating remapped storage.
*
*   INPUTS
*       Initialize only, BOOL. Default FALSE. No local setter or getter.
*
*   NOTES
*       TRUE passes the screen bitmap to AllocBitMap and adds BMF_MINPLANES. The
*       requested depth is capped at eight; the friend affects actual allocation
*       behavior. This affects only remapping allocation and acquires no
*       ownership of the screen bitmap.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_BitmapData
{
    struct BitMap *bm, *remapped_bm;
    ULONG *sourcecolors;
    LONG width, height, precision, transparent;
    UBYTE *mappingtable;
    PLANEPTR mask;
    WORD *remaptable;
    BYTE usefriend;
    ULONG alpha;
    BOOL use_alpha;
};

/*
 * Optionally allocate and populate indexed remapped bitmap from borrowed
 * source/tables. Own destination, optional mask and cached pens until
 * Cleanup; keep remap table until Dispose. Requires live screen and bounded
 * dimensions/tables. Returns no status; allocation/pen failures and narrow
 * counters have class-documented limits. Frees temporary line and raster
 * resources on completed paths.
 */
static void remap_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);
    struct BitMap *friendbm = NULL;
    struct RastPort temprp, bmrp, *scrrp;
    UBYTE *linebuffer;
    ULONG *cgfxcoltab = NULL;
    LONG bmflags = 0;
    WORD bmdepth, bmwidth, bmheight, bmcols, x, y;

    if (!data->mappingtable && !data->sourcecolors)
        return;
    if (!data->bm || (data->width < 1) || (data->height < 1))
        return;

    /* Don't remap if bitmap is hicolor/truecolor */
    if (GetBitMapAttr(data->bm, BMA_DEPTH) > 8)
        return;

    if (!data->mappingtable && !data->remaptable)
    {
        data->remaptable =
            AllocVec(256 * sizeof(WORD), MEMF_PUBLIC | MEMF_CLEAR);
        if (!data->remaptable)
            return;
    }

    scrrp = &_screen(obj)->RastPort;

    if (data->usefriend)
    {
        friendbm = scrrp->BitMap;
        bmflags |= BMF_MINPLANES;
    }

    linebuffer = AllocVec(data->width + 16, MEMF_PUBLIC);
    if (!linebuffer)
        return;

    bmdepth = GetBitMapAttr(scrrp->BitMap, BMA_DEPTH);
    if (bmdepth > 8)
        bmdepth = 8;

    bmcols = 1L << bmdepth;

    data->remapped_bm =
        AllocBitMap(data->width, data->height, bmdepth, bmflags, friendbm);

    if (!data->remapped_bm)
    {
        FreeVec(linebuffer);
        return;
    }

    bmwidth = GetBitMapAttr(data->remapped_bm, BMA_WIDTH);
    bmheight = GetBitMapAttr(data->remapped_bm, BMA_HEIGHT);

    if (data->transparent != -1)
    {
        data->mask = AllocRaster(bmwidth, bmheight);
        memset(data->mask, 0xff, RASSIZE(bmwidth, bmheight));
    }

    if (CyberGfxBase &&
        !data->mappingtable &&
        (GetBitMapAttr(data->remapped_bm, BMA_DEPTH) >= 15))
    {
        cgfxcoltab = AllocVec(bmcols * sizeof(ULONG), MEMF_ANY);

        if (cgfxcoltab)
        {
            for (y = 0; y < bmcols; y++)
            {
                ULONG red = data->sourcecolors[y * 3] & 0xFF000000;
                ULONG green = data->sourcecolors[y * 3 + 1] & 0xFF000000;
                ULONG blue = data->sourcecolors[y * 3 + 2] & 0xFF000000;

                cgfxcoltab[y] = (red >> 8) | (green >> 16) | (blue >> 24);
            }
        }

    }

    InitRastPort(&temprp);
    temprp.BitMap = AllocBitMap(data->width, 1, 1, 0, NULL);

    InitRastPort(&bmrp);

    for (y = 0; y < data->height; y++)
    {
        /* Read a line from source bitmap */

        bmrp.BitMap = data->bm;
        if (temprp.BitMap)
        {
            ReadPixelLine8(&bmrp, 0, y, data->width, linebuffer, &temprp);
        }
        else
        {
            for (x = 0; x < data->width; x++)
            {
                linebuffer[x] = ReadPixel(&bmrp, x, y);
            }
        }

        /* Build the mask, totaly slow but works */
        if (data->mask)
        {
            UBYTE *mask = data->mask + y * bmwidth / 8;
            UBYTE xmask = 0x80;

            for (x = 0; x < data->width; x++)
            {
                if (linebuffer[x] == data->transparent)
                {
                    *mask &= ~xmask;
                }

                xmask >>= 1;
                if (!xmask)
                {
                    xmask = 0x80;
                    mask++;
                }
            }
        }

        /* Remap the line */
        if (data->mappingtable)
        {
            for (x = 0; x < data->width; x++)
            {
                linebuffer[x] = data->mappingtable[linebuffer[x]];
            }
        }
        else if (!cgfxcoltab)
        {
            for (x = 0; x < data->width; x++)
            {
                UBYTE pixel = linebuffer[x];
                UBYTE remappixel = data->remaptable[pixel];

                if (!remappixel)
                {
                    struct TagItem tags[3];
                    tags[0].ti_Tag = OBP_Precision;
                    tags[0].ti_Data = data->precision;
                    tags[1].ti_Tag = OBP_FailIfBad;
                    tags[1].ti_Data = FALSE;
                    tags[2].ti_Tag = 0;

                    data->remaptable[pixel] = remappixel =
                        ObtainBestPenA(_screen(obj)->ViewPort.ColorMap,
                        data->sourcecolors[pixel * 3],
                        data->sourcecolors[pixel * 3 + 1],
                        data->sourcecolors[pixel * 3 + 2], tags) | 0x100;
                }

                linebuffer[x] = (remappixel & 0xFF);
            }

        }

        /* Write line into destination bitmap */

        bmrp.BitMap = data->remapped_bm;

        if (cgfxcoltab)
        {
            WriteLUTPixelArray(linebuffer,
                0,
                0,
                data->width,
                &bmrp, cgfxcoltab, 0, y, data->width, 1, CTABFMT_XRGB8);
        }
        else
        {
            bmrp.BitMap = data->remapped_bm;
            if (temprp.BitMap)
            {
                WritePixelLine8(&bmrp, 0, y, data->width, linebuffer,
                    &temprp);
            }
            else
            {
                for (x = 0; x < data->width; x++)
                {
                    SetAPen(&bmrp, linebuffer[x]);
                    WritePixel(&bmrp, x, y);
                }
            }

        }

    }

    if (temprp.BitMap)
    {
        WaitBlit();
        FreeBitMap(temprp.BitMap);
    }

    FreeVec(cgfxcoltab);
    FreeVec(linebuffer);
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Construct Area, initialize precision/transparency/alpha defaults and
 * store borrowed source pointers and dimensions. Return object or zero on
 * superclass failure. No remapping yet; Alpha tag presence enables
 * unimplemented image path.
 */
IPTR Bitmap__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BitmapData *data;
    struct TagItem *tag, *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    data->precision = PRECISION_GUI;
    data->transparent = -1;
    data->alpha = 0xffffffff;
    data->use_alpha = FALSE;

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Bitmap_Alpha:
            data->alpha = tag->ti_Data;
            data->use_alpha = TRUE;
            break;

        case MUIA_Bitmap_Bitmap:
            data->bm = (struct BitMap *)tag->ti_Data;
            break;

        case MUIA_Bitmap_Height:
            data->height = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_MappingTable:
            data->mappingtable = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_Bitmap_Precision:
            data->precision = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_SourceColors:
            data->sourcecolors = (ULONG *) tag->ti_Data;
            break;

        case MUIA_Bitmap_Transparent:
            data->transparent = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_UseFriend:
            data->usefriend = (tag->ti_Data != 0);
            break;

        case MUIA_Bitmap_Width:
            data->width = (LONG) tag->ti_Data;
            break;

        }
    }

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Wait for blits and free any remaining remapped bitmap, then free remap
 * table and return Area result. Requires prior Cleanup for mask and pens.
 * Source bitmap and caller tables remain borrowed.
 */
IPTR Bitmap__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (data->remapped_bm)
    {
        WaitBlit();
        FreeBitMap(data->remapped_bm);
    }
    if (data->remaptable)
        FreeVec(data->remaptable);

    return DoSuperMethodA(cl, obj, msg);
}

/**************************************************************************
 OM_SET
**************************************************************************/
/*
 * Store supported values, except ignore source bitmap replacement while
 * remapped storage exists. Forward original tags and return Area result. No
 * remap, redraw or relayout; caller must keep resources consistent. Alpha
 * tag enables alpha mode regardless of value.
 */
IPTR Bitmap__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Bitmap_Alpha:
            data->alpha = tag->ti_Data;
            data->use_alpha = TRUE;
            break;

        case MUIA_Bitmap_Bitmap:
            if (!data->remapped_bm)
            {
                data->bm = (struct BitMap *)tag->ti_Data;
            }
            break;

        case MUIA_Bitmap_Height:
            data->height = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_MappingTable:
            data->mappingtable = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_Bitmap_Precision:
            data->precision = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_SourceColors:
            data->sourcecolors = (ULONG *) tag->ti_Data;
            break;

        case MUIA_Bitmap_Transparent:
            data->transparent = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Width:
            data->width = (LONG) tag->ti_Data;
            break;

        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 OM_GET
**************************************************************************/
/*
 * Write supported stored attributes or remapped pointer into valid caller-
 * owned IPTR storage and return TRUE. Delegate others including UseFriend
 * to Area. No ownership transfer, allocation or state change.
 */
IPTR Bitmap__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Bitmap_Alpha:
        STORE = data->alpha;
        return TRUE;

    case MUIA_Bitmap_Bitmap:
        STORE = (IPTR) data->bm;
        return TRUE;

    case MUIA_Bitmap_Height:
        STORE = (IPTR) data->height;
        return TRUE;

    case MUIA_Bitmap_MappingTable:
        STORE = (IPTR) data->mappingtable;
        return TRUE;

    case MUIA_Bitmap_Precision:
        STORE = (IPTR) data->precision;
        return TRUE;

    case MUIA_Bitmap_RemappedBitmap:
        STORE = (IPTR) data->remapped_bm;
        return TRUE;

    case MUIA_Bitmap_SourceColors:
        STORE = (IPTR) data->sourcecolors;
        return TRUE;

    case MUIA_Bitmap_Transparent:
        STORE = (IPTR) data->transparent;
        return TRUE;

    case MUIA_Bitmap_Width:
        STORE = (IPTR) data->width;
        return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Call Area and return FALSE on failure; otherwise attempt remapping and
 * return TRUE without propagating remap allocation failures. Requires live
 * screen/source resources and paired Cleanup.
 */
IPTR Bitmap__MUIM_Setup(struct IClass *cl, Object *obj, Msg msg)
{
    //struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
        return FALSE;

    remap_bitmap(cl, obj);

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Release owned mask using remapped dimensions, wait/free remapped bitmap,
 * and release nonzero cached pen entries through live screen ColorMap.
 * Clear released state and return Area Cleanup result. Keep remap table for
 * reuse; borrowed source/tables are not freed.
 */
IPTR Bitmap__MUIM_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);

    if (data->mask)
    {
        LONG bmwidth = GetBitMapAttr(data->remapped_bm, BMA_WIDTH);
        LONG bmheight = GetBitMapAttr(data->remapped_bm, BMA_HEIGHT);
        FreeRaster(data->mask, bmwidth, bmheight);

        data->mask = NULL;
    }

    if (data->remapped_bm)
    {
        WaitBlit();
        FreeBitMap(data->remapped_bm);
        data->remapped_bm = NULL;

        if (data->remaptable)
        {
            WORD i;

            for (i = 0; i < 256; i++)
            {
                if (data->remaptable[i])
                {
                    ReleasePen(_screen(obj)->ViewPort.ColorMap,
                        data->remaptable[i] & 0xFF);
                    data->remaptable[i] = 0;
                }
            }
        }
    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
}

/**************************************************************************
 MUIM_AskMinMax
**************************************************************************/
/*
 * Call Area, add one pixel to minimum/default width and height and set
 * maxima to MUI_MAXMAX. Return TRUE. Requires writable size structure;
 * configured image dimensions do not influence this calculation.
 */
IPTR Bitmap__MUIM_AskMinMax(struct IClass *cl, Object *obj,
    struct MUIP_AskMinMax *msg)
{
    DoSuperMethodA(cl, obj, (Msg) msg);

    msg->MinMaxInfo->MinWidth += 1;
    msg->MinMaxInfo->MinHeight += 1;

    msg->MinMaxInfo->DefWidth += 1;
    msg->MinMaxInfo->DefHeight += 1;

    msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
    msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

    return TRUE;
}

/**************************************************************************
 MUIM_Draw
**************************************************************************/
/*
 * Call Area then blit chosen remapped/original bitmap at inner top-left,
 * cropping positive configured dimensions to content size. Alpha mode
 * performs no blit. Return TRUE without local flag filtering or graphics
 * status checks. Requires valid source coverage, mask and render context.
 */
IPTR Bitmap__MUIM_Draw(struct IClass *cl, Object *obj,
    struct MUIP_Draw *msg)
{
    struct MUI_BitmapData *data = INST_DATA(cl, obj);
    struct BitMap *bm;

    DoSuperMethodA(cl, obj, (Msg) msg);

    bm = data->remapped_bm ? data->remapped_bm : data->bm;
    if (bm)
    {
        LONG width, height;

        width = data->width;
        height = data->height;

        if (width > _mwidth(obj))
            width = _mwidth(obj);
        if (height > _mheight(obj))
            height = _mheight(obj);

        if ((width > 0) && (height > 0))
        {
            if (data->use_alpha)
            {
                // TODO: implement
            }
            else if (data->mask)
            {
                BltMaskBitMapRastPort(bm, 0, 0, _rp(obj), _mleft(obj),
                    _mtop(obj), width, height, 0xE0, data->mask);
            }
            else
            {
                BltBitMapRastPort(bm, 0, 0, _rp(obj), _mleft(obj),
                    _mtop(obj), width, height, 0xC0);
            }
        }
    }

    return TRUE;
}


/*
 * Dispatch local overrides and return their results; delegate other
 * messages to the declared superclass. Requires ABI-correct message and
 * live object except construction. Ownership follows the class lifecycle
 * contract.
 */
BOOPSI_DISPATCHER(IPTR, Bitmap_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Bitmap__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Bitmap__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Bitmap__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Bitmap__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Bitmap__MUIM_Setup(cl, obj, msg);
    case MUIM_Cleanup:
        return Bitmap__MUIM_Cleanup(cl, obj, msg);
    case MUIM_AskMinMax:
        return Bitmap__MUIM_AskMinMax(cl, obj,
            (struct MUIP_AskMinMax *)msg);
    case MUIM_Draw:
        return Bitmap__MUIM_Draw(cl, obj, (struct MUIP_Draw *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}
BOOPSI_DISPATCHER_END

/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Bitmap_desc =
{
    MUIC_Bitmap,
    MUIC_Area,
    sizeof(struct MUI_BitmapData),
    (void *) Bitmap_Dispatcher
};
