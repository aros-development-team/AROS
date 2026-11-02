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

/****** Bodychunk.mui/MUIC_Bodychunk ************
*
*   NAME
*       MUIC_Bodychunk
*
*   SUPERCLASS
*       Bitmap.mui
*
*   DESCRIPTION
*       Decode borrowed ILBM-style BODY data into an owned bitmap during Setup,
*       then display it through Bitmap. This class does not read an IFF file,
*       parse chunk headers or infer dimensions. Supply Bitmap_Width,
*       Bitmap_Height, Bodychunk_Depth and Bodychunk_Body consistently;
*       dimensions and depth default to zero, Body to NULL. Compression and
*       Masking default to zero.
*       Rows contain word-aligned plane bytes: bytes per plane row equal ((width
*       + 15) & ~15) / 8. Color planes are interleaved by row, followed by one
*       extra plane only when Masking equals 1. Compression 0 reads that layout
*       directly; compression 1 expands ByteRun1 data for the combined planes of
*       each row. The extra mask plane is consumed but not used for drawing
*       transparency. Use Bitmap's separate transparency mechanism if needed.
*
*   METHODS
*       Setup rejects NULL Body, nonpositive dimensions, depth outside 1..8, or
*       stored Compression above 1. It returns FALSE on those checks or required
*       allocation failure. It creates the source bitmap before calling Bitmap
*       Setup, supplies it through Bitmap_Bitmap, and frees it if superclass
*       setup fails. Temporary scanline bitmap allocation can fall back to
*       per-pixel writes.
*       Cleanup first invokes Bitmap Cleanup, clears Bitmap_Bitmap and then
*       frees the decoded bitmap after WaitBlit. Dispose frees any remaining
*       decoded bitmap, but does not replace the complete superclass cleanup
*       lifecycle. Body is never freed. Setters update stored fields only; they
*       do not decode again, request redraw or relayout. Width/Height tags
*       update both this class and Bitmap, so live changes can disagree with the
*       old decoded bitmap.
*
*   BUGS
*       There is no encoded-data length parameter or decoder input bound.
*       ByteRun1 -128 is a no-op and malformed streams can read indefinitely
*       beyond input; a run ending beyond the requested output is cut short
*       without structural validation. Caller-provided data must be complete and
*       correctly encoded.
*       The decompression workspace has width + 16 bytes available, but a
*       decoded row needs bytes-per-plane-row times total plane count. That
*       requirement is not checked and can exceed the buffer, including depth
*       eight plus a mask at some widths. Planar conversion also prefetches the
*       next source byte at eight-pixel boundaries, so it can read one byte past
*       the final required plane data. Dimensions use WORD iteration and
*       unchecked arithmetic. This implementation is not a bounds-checked
*       decoder for arbitrary BODY data.
*
*/
/****** Bodychunk.mui/MUIA_Bodychunk_Body ************
*
*   NAME
*       MUIA_Bodychunk_Body
*
*   FUNCTION
*       Supply or retrieve the encoded BODY bytes.
*
*   INPUTS
*       Initialize, set and get, UBYTE pointer. Default NULL.
*
*   NOTES
*       The buffer is borrowed, never copied or freed. Keep complete readable
*       data available for every Setup; changing it while a decoded bitmap
*       exists does not update that image. Get returns the original pointer. No
*       length is stored or checked. Provide readable padding for the planar
*       converter prefetch as described in the class entry.
*
*/
/****** Bodychunk.mui/MUIA_Bodychunk_Compression ************
*
*   NAME
*       MUIA_Bodychunk_Compression
*
*   FUNCTION
*       Select raw or ByteRun1 decoding.
*
*   INPUTS
*       Initialize, set and get, UBYTE. Default 0. Stored 0 means raw; 1 means
*       ByteRun1; larger stored values make Setup fail.
*
*   NOTES
*       Tag data is converted to UBYTE before validation, so values outside that
*       type are truncated. Set does not decode again. ByteRun1 uses nonnegative
*       control bytes for literal runs, -1..-127 for repeated-byte runs and -128
*       as a no-op, without checking encoded input bounds.
*
*/
/****** Bodychunk.mui/MUIA_Bodychunk_Depth ************
*
*   NAME
*       MUIA_Bodychunk_Depth
*
*   FUNCTION
*       Set or retrieve the number of color bitplanes.
*
*   INPUTS
*       Initialize, set and get, LONG. Default zero; Setup accepts 1 through 8.
*
*   NOTES
*       The decoded bitmap uses this color depth. Masking 1 adds a separate
*       consumed plane to the input row but does not increase decoded color
*       depth. Set performs no immediate validation or regeneration. Supply
*       dimensions, plane data and optional palette consistent with this value.
*
*/
/****** Bodychunk.mui/MUIA_Bodychunk_Masking ************
*
*   NAME
*       MUIA_Bodychunk_Masking
*
*   FUNCTION
*       Indicate whether each source row includes an extra mask plane.
*
*   INPUTS
*       Initialize, set and get, UBYTE. Default 0.
*
*   NOTES
*       Only value 1 has special handling: one extra plane is consumed after the
*       color planes. It is discarded, not converted to a drawing mask. Other
*       stored values are treated as no extra plane and are not rejected. Set
*       does not regenerate decoded data. This attribute does not itself select
*       a transparent color or enable Bitmap alpha drawing.
*
*/

extern struct Library *MUIMasterBase;

struct MUI_BodychunkData
{
    struct BitMap *bm;
    UBYTE *body;
    LONG width, height, depth;
    UBYTE compression;
    UBYTE masking;
};

/*
 * Convert color planes of one row to byte indices in caller-owned
 * destination. Requires valid source planes, destination width bytes and
 * readable prefetch padding. Width/depth are WORD and depth must be 1..8.
 * Allocates nothing and returns no status; extra mask plane is ignored by
 * caller.
 */
static void planar2chunky(UBYTE *src, UBYTE *dest, WORD width, WORD depth,
    LONG bpr)
{
    UBYTE *s, *d, byte;
    UBYTE pmask, dmask = 1, notdmask = ~1;
    WORD x, pl;

    for (pl = 0; pl < depth; pl++)
    {
        pmask = 0x80;
        s = src;
        d = dest;

        byte = *s++;

        for (x = 0; x < width; x++)
        {
            if (byte & pmask)
            {
                *d++ |= dmask;
            }
            else
            {
                *d++ &= notdmask;
            }

            if (pmask == 0x1)
            {
                pmask = 0x80;
                byte = *s++;
            }
            else
            {
                pmask >>= 1;
            }
        }

        dmask <<= 1;
        notdmask = ~dmask;

        src += bpr;
    }
}

/*
 * Decode ByteRun1 until requested output bytes have been produced; return
 * advanced source pointer. Requires positive output count, sufficient
 * destination and valid complete encoded stream. No input-length guard;
 * -128 emits nothing and malformed input can read without bound. Runs are
 * cut at output boundary without validation.
 */
static UBYTE *unpack_byterun1(UBYTE *src, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;

    for (;;)
    {
        c = (BYTE) (*src++);
        if (c >= 0)
        {
            while (c-- >= 0)
            {
                *dest++ = *src++;
                if (--unpackedsize <= 0)
                    return src;
            }
        }
        else if (c != -128)
        {
            c = -c;
            r = *src++;

            while (c-- >= 0)
            {
                *dest++ = r;
                if (--unpackedsize <= 0)
                    return src;
            }
        }
    }

}

/*
 * Validate basic input, allocate owned bitmap and temporary scanline
 * workspace, decode all rows and write pixels. Return FALSE on rejected
 * parameters or required allocation failure, TRUE after conversion.
 * Requires complete borrowed BODY and representable dimensions;
 * decompressed-row capacity is not checked. Frees temporary resources,
 * retains decoded bitmap.
 */
static BOOL make_bitmap(struct IClass *cl, Object *obj)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);
    struct RastPort bmrp, temprp;
    UBYTE *chunkyline;
    UBYTE *bodyptr, *unpackptr;
    LONG w16, bpr, bpl;
    WORD x, y, totdepth;

    if (!data->body ||
        (data->width < 1) ||
        (data->height < 1) ||
        (data->depth < 1) || (data->depth > 8) || (data->compression > 1))
    {
        return FALSE;
    }

    chunkyline = AllocVec(2 * (data->width + 16), MEMF_PUBLIC | MEMF_CLEAR);
    if (!chunkyline)
        return FALSE;

    unpackptr = chunkyline + data->width + 16;

    data->bm =
        AllocBitMap(data->width, data->height, data->depth, BMF_CLEAR,
        NULL);
    if (!data->bm)
    {
        FreeVec(chunkyline);
        return FALSE;
    }

    InitRastPort(&temprp);
    temprp.BitMap = AllocBitMap(data->width, 1, 1, 0, NULL);

    InitRastPort(&bmrp);
    bmrp.BitMap = data->bm;

    bodyptr = data->body;
    w16 = (data->width + 15) & ~15;
    bpr = w16 / 8;

    totdepth = data->depth + ((data->masking == 1) ? 1 : 0);
    bpl = bpr * totdepth;

    for (y = 0; y < data->height; y++)
    {
        if (data->compression)
        {
            bodyptr = unpack_byterun1(bodyptr, unpackptr, bpl);
            planar2chunky(unpackptr, chunkyline, data->width, data->depth,
                bpr);
        }
        else
        {
            planar2chunky(bodyptr, chunkyline, data->width, data->depth,
                bpr);
            bodyptr += bpl;
        }

        if (temprp.BitMap)
        {
            WritePixelLine8(&bmrp, 0, y, data->width, chunkyline, &temprp);
        }
        else
        {
            for (x = 0; x < data->width; x++)
            {
                SetAPen(&bmrp, chunkyline[x]);
                WritePixel(&bmrp, x, y);
            }
        }
    }

    if (temprp.BitMap)
    {
        WaitBlit();
        FreeBitMap(temprp.BitMap);
    }

    FreeVec(chunkyline);

    return TRUE;
}

/**************************************************************************
 OM_NEW
**************************************************************************/
/*
 * Construct Bitmap and store borrowed BODY plus decode fields and local
 * dimensions. Return object or zero on superclass failure. No decoding or
 * validation occurs yet; BODY remains caller-owned.
 */
IPTR Bodychunk__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BodychunkData *data;
    struct TagItem *tag, *tags;

    obj = (Object *) DoSuperMethodA(cl, obj, (Msg) msg);
    if (!obj)
        return FALSE;

    data = INST_DATA(cl, obj);

    /* parse initial taglist */

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags));)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Bodychunk_Body:
            data->body = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Compression:
            data->compression = (UBYTE) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Depth:
            data->depth = (LONG) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Masking:
            data->masking = (UBYTE) tag->ti_Data;
            break;

        case MUIA_Bitmap_Width:
            data->width = (LONG) tag->ti_Data;
            break;

        case MUIA_Bitmap_Height:
            data->height = (LONG) tag->ti_Data;
            break;

        }
    }

    return (IPTR) obj;
}

/**************************************************************************
 OM_DISPOSE
**************************************************************************/
/*
 * Wait for blits, free remaining owned decoded bitmap and return Bitmap
 * disposal result. Requires prior normal Cleanup for superclass mask/pens.
 * Never frees borrowed BODY.
 */
IPTR Bodychunk__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

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
 * Store BODY/decode fields and local dimensions, then return Bitmap setter
 * result. No decoding, validation, redraw or relayout; live dimension
 * changes can conflict with existing bitmap storage. BODY is borrowed.
 */
IPTR Bodychunk__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
        case MUIA_Bodychunk_Body:
            data->body = (UBYTE *) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Compression:
            data->compression = (UBYTE) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Depth:
            data->depth = (LONG) tag->ti_Data;
            break;

        case MUIA_Bodychunk_Masking:
            data->masking = (UBYTE) tag->ti_Data;
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
 * Write stored BODY/decode attributes into valid caller-owned IPTR storage
 * and return TRUE. Delegate others to Bitmap. No allocation, validation or
 * ownership transfer.
 */
IPTR Bodychunk__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
#define STORE *(msg->opg_Storage)

    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

    switch (msg->opg_AttrID)
    {
    case MUIA_Bodychunk_Body:
        STORE = (IPTR) data->body;
        return TRUE;

    case MUIA_Bodychunk_Compression:
        STORE = (IPTR) data->compression;
        return TRUE;

    case MUIA_Bodychunk_Depth:
        STORE = (IPTR) data->depth;
        return TRUE;

    case MUIA_Bodychunk_Masking:
        STORE = (IPTR) data->masking;
        return TRUE;

    }

    return DoSuperMethodA(cl, obj, (Msg) msg);
#undef STORE
}

/**************************************************************************
 MUIM_Setup
**************************************************************************/
/*
 * Decode owned source bitmap, install it through Bitmap_Bitmap and invoke
 * Bitmap Setup. Return FALSE on decode failure or superclass failure; on
 * latter clear source and free decoded bitmap. Requires matching lifecycle
 * and complete valid borrowed data. TRUE does not prove remap success.
 */
IPTR Bodychunk__MUIM_Setup(struct IClass *cl, Object *obj,
    struct MUIP_Setup *msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);

    if (!make_bitmap(cl, obj))
        return FALSE;

    set(obj, MUIA_Bitmap_Bitmap, (IPTR) data->bm);

    if (!DoSuperMethodA(cl, obj, (Msg) msg))
    {
        set(obj, MUIA_Bitmap_Bitmap, NULL);
        WaitBlit();
        FreeBitMap(data->bm);
        data->bm = NULL;

        return FALSE;
    }

    return TRUE;
}

/**************************************************************************
 MUIM_Cleanup
**************************************************************************/
/*
 * Invoke Bitmap Cleanup first, then clear source attribute, wait for blits
 * and free decoded bitmap if present. Return superclass cleanup result.
 * Requires live context; BODY remains borrowed and available for future
 * Setup.
 */
IPTR Bodychunk__MUIM_Cleanup(struct IClass *cl, Object *obj,
    struct MUIP_Cleanup *msg)
{
    struct MUI_BodychunkData *data = INST_DATA(cl, obj);
    IPTR retval;

    retval = DoSuperMethodA(cl, obj, (Msg) msg);
    if (data->bm)
    {
        set(obj, MUIA_Bitmap_Bitmap, NULL);

        WaitBlit();

        FreeBitMap(data->bm);
        data->bm = NULL;
    }

    return retval;
}


/*
 * Dispatch local overrides and return their results; delegate other
 * messages to the declared superclass. Requires ABI-correct message and
 * live object except construction. Ownership follows the class lifecycle
 * contract.
 */
BOOPSI_DISPATCHER(IPTR, Bodychunk_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
    case OM_NEW:
        return Bodychunk__OM_NEW(cl, obj, (struct opSet *)msg);
    case OM_DISPOSE:
        return Bodychunk__OM_DISPOSE(cl, obj, msg);
    case OM_SET:
        return Bodychunk__OM_SET(cl, obj, (struct opSet *)msg);
    case OM_GET:
        return Bodychunk__OM_GET(cl, obj, (struct opGet *)msg);
    case MUIM_Setup:
        return Bodychunk__MUIM_Setup(cl, obj, (struct MUIP_Setup *)msg);
    case MUIM_Cleanup:
        return Bodychunk__MUIM_Cleanup(cl, obj, (struct MUIP_Cleanup *)msg);
    }

    return DoSuperMethodA(cl, obj, msg);
}

BOOPSI_DISPATCHER_END
/*
 * Class descriptor.
 */
const struct __MUIBuiltinClass _MUI_Bodychunk_desc =
{
    MUIC_Bodychunk,
    MUIC_Bitmap,
    sizeof(struct MUI_BodychunkData),
    (void *) Bodychunk_Dispatcher
};
