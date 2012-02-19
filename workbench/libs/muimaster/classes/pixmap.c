/*
    Copyright © 2011, Thore Böckelmann. All rights reserved.
    Copyright © 2012, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/utility.h>

#include <libraries/mui.h>
#include <cybergraphx/cybergraphics.h>

#include <strings.h>
#include <bzlib.h>

#include "pixmap.h"
#include "pixmap_private.h"

#include <aros/debug.h>

#ifndef MEMF_SHARED
#define MEMF_SHARED MEMF_ANY
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

// woraround for missing Zune functionality
static BOOL _isfloating(Object *obj)
{
    return FALSE;
}

static BOOL _isdisabled(Object *obj)
{
    return FALSE;
}

static void MUIP_DrawDisablePattern(struct MUI_RenderInfo *mri, LONG left, LONG top, LONG width, LONG height)
{
}


// libbz2_nostdio needs this
void  free(void *memory)
{
    FreeVec(memory);
}

void *malloc(size_t size)
{
    return AllocVec(size, MEMF_ANY);
}

void bz_internal_error(int errcode)
{
    bug("[Pixmap.mui/bz_internal_error] errcode %d\n", errcode);
}

/* ------------------------------------------------------------------------- */

/// default color map
const ULONG defaultColorMap[256] =
{
    0x00000000, 0x00000055, 0x000000aa, 0x000000ff, 0x00002400, 0x00002455, 0x000024aa, 0x000024ff, 0x00004900, 0x00004955, 0x000049aa, 0x000049ff, 0x00006d00, 0x00006d55, 0x00006daa, 0x00006dff,
    0x00009200, 0x00009255, 0x000092aa, 0x000092ff, 0x0000b600, 0x0000b655, 0x0000b6aa, 0x0000b6ff, 0x0000db00, 0x0000db55, 0x0000dbaa, 0x0000dbff, 0x0000ff00, 0x0000ff55, 0x0000ffaa, 0x0000ffff,
    0x00240000, 0x00240055, 0x002400aa, 0x002400ff, 0x00242400, 0x00242455, 0x002424aa, 0x002424ff, 0x00244900, 0x00244955, 0x002449aa, 0x002449ff, 0x00246d00, 0x00246d55, 0x00246daa, 0x00246dff,
    0x00249200, 0x00249255, 0x002492aa, 0x002492ff, 0x0024b600, 0x0024b655, 0x0024b6aa, 0x0024b6ff, 0x0024db00, 0x0024db55, 0x0024dbaa, 0x0024dbff, 0x0024ff00, 0x0024ff55, 0x0024ffaa, 0x0024ffff,
    0x00490000, 0x00490055, 0x004900aa, 0x004900ff, 0x00492400, 0x00492455, 0x004924aa, 0x004924ff, 0x00494900, 0x00494955, 0x004949aa, 0x004949ff, 0x00496d00, 0x00496d55, 0x00496daa, 0x00496dff,
    0x00499200, 0x00499255, 0x004992aa, 0x004992ff, 0x0049b600, 0x0049b655, 0x0049b6aa, 0x0049b6ff, 0x0049db00, 0x0049db55, 0x0049dbaa, 0x0049dbff, 0x0049ff00, 0x0049ff55, 0x0049ffaa, 0x0049ffff,
    0x006d0000, 0x006d0055, 0x006d00aa, 0x006d00ff, 0x006d2400, 0x006d2455, 0x006d24aa, 0x006d24ff, 0x006d4900, 0x006d4955, 0x006d49aa, 0x006d49ff, 0x006d6d00, 0x006d6d55, 0x006d6daa, 0x006d6dff,
    0x006d9200, 0x006d9255, 0x006d92aa, 0x006d92ff, 0x006db600, 0x006db655, 0x006db6aa, 0x006db6ff, 0x006ddb00, 0x006ddb55, 0x006ddbaa, 0x006ddbff, 0x006dff00, 0x006dff55, 0x006dffaa, 0x006dffff,
    0x00920000, 0x00920055, 0x009200aa, 0x009200ff, 0x00922400, 0x00922455, 0x009224aa, 0x009224ff, 0x00924900, 0x00924955, 0x009249aa, 0x009249ff, 0x00926d00, 0x00926d55, 0x00926daa, 0x00926dff,
    0x00929200, 0x00929255, 0x009292aa, 0x009292ff, 0x0092b600, 0x0092b655, 0x0092b6aa, 0x0092b6ff, 0x0092db00, 0x0092db55, 0x0092dbaa, 0x0092dbff, 0x0092ff00, 0x0092ff55, 0x0092ffaa, 0x0092ffff,
    0x00b60000, 0x00b60055, 0x00b600aa, 0x00b600ff, 0x00b62400, 0x00b62455, 0x00b624aa, 0x00b624ff, 0x00b64900, 0x00b64955, 0x00b649aa, 0x00b649ff, 0x00b66d00, 0x00b66d55, 0x00b66daa, 0x00b66dff,
    0x00b69200, 0x00b69255, 0x00b692aa, 0x00b692ff, 0x00b6b600, 0x00b6b655, 0x00b6b6aa, 0x00b6b6ff, 0x00b6db00, 0x00b6db55, 0x00b6dbaa, 0x00b6dbff, 0x00b6ff00, 0x00b6ff55, 0x00b6ffaa, 0x00b6ffff,
    0x00db0000, 0x00db0055, 0x00db00aa, 0x00db00ff, 0x00db2400, 0x00db2455, 0x00db24aa, 0x00db24ff, 0x00db4900, 0x00db4955, 0x00db49aa, 0x00db49ff, 0x00db6d00, 0x00db6d55, 0x00db6daa, 0x00db6dff,
    0x00db9200, 0x00db9255, 0x00db92aa, 0x00db92ff, 0x00dbb600, 0x00dbb655, 0x00dbb6aa, 0x00dbb6ff, 0x00dbdb00, 0x00dbdb55, 0x00dbdbaa, 0x00dbdbff, 0x00dbff00, 0x00dbff55, 0x00dbffaa, 0x00dbffff,
    0x00ff0000, 0x00ff0055, 0x00ff00aa, 0x00ff00ff, 0x00ff2400, 0x00ff2455, 0x00ff24aa, 0x00ff24ff, 0x00ff4900, 0x00ff4955, 0x00ff49aa, 0x00ff49ff, 0x00ff6d00, 0x00ff6d55, 0x00ff6daa, 0x00ff6dff,
    0x00ff9200, 0x00ff9255, 0x00ff92aa, 0x00ff92ff, 0x00ffb600, 0x00ffb655, 0x00ffb6aa, 0x00ffb6ff, 0x00ffdb00, 0x00ffdb55, 0x00ffdbaa, 0x00ffdbff, 0x00ffff00, 0x00ffff55, 0x00ffffaa, 0x00ffffff
};

///

IPTR Pixmap__OM_NEW(struct IClass *cl, Object *obj, struct opSet *msg)
{
    const struct TagItem *tags;
    struct TagItem *tag;

    if((obj = (Object *)DoSuperMethodA(cl, obj, (Msg)msg)) != NULL)
    {
        struct Pixmap_DATA *data = INST_DATA(cl, obj);

        data->format = MUIV_Pixmap_Format_ARGB32;
        data->alpha = 0xffffffffUL;
        data->compression = MUIV_Pixmap_Compression_None;

        for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
        {
            switch (tag->ti_Tag)
            {
                case MUIA_LeftEdge:              data->leftOffset = tag->ti_Data; break;
                case MUIA_TopEdge:               data->topOffset = tag->ti_Data; break;
                case MUIA_Pixmap_Data:           data->data = (APTR)tag->ti_Data; break;
                case MUIA_Pixmap_Format:         data->format = tag->ti_Data; break;
                case MUIA_Pixmap_Width:          data->width  = tag->ti_Data; break;
                case MUIA_Pixmap_Height:         data->height = tag->ti_Data; break;
                case MUIA_Pixmap_CLUT:           data->clut = (APTR)tag->ti_Data; break;
                case MUIA_Pixmap_Alpha:          data->alpha = tag->ti_Data; break;
                case MUIA_Pixmap_Compression:    data->compression = tag->ti_Data; break;
                case MUIA_Pixmap_CompressedSize: data->compressedSize = tag->ti_Data; break;
            }
        }
    }

    return (IPTR)obj;
}


static void FreeImage(struct IClass *cl, Object *obj)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    if  (data->uncompressedData != NULL && data->uncompressedData != data->data)
    {
        FreeVec(data->uncompressedData);
        data->uncompressedData = NULL;
    }
}

IPTR Pixmap__OM_DISPOSE(struct IClass *cl, Object *obj, Msg msg)
{
    FreeImage(cl, obj);
    return DoSuperMethodA(cl, obj, msg);
}


#define RAWIDTH(w)                      ((((UWORD)(w))+15)>>3 & 0xFFFE)

static void DitherImage(struct IClass *cl, Object *obj)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    if ((data->ditheredData = AllocVec(data->width * data->height, MEMF_SHARED)) != NULL)
    {
        UBYTE *mask = NULL;
        UBYTE *mPtr = NULL;
        LONG y;
        UBYTE *dataPtr = (UBYTE *)data->uncompressedData;
        UBYTE *ditheredPtr = (UBYTE *)data->ditheredData;
        const ULONG *colorMap = (data->clut != NULL) ? data->clut : defaultColorMap;

        // only ARGB raw data contain transparency data, hence we need to
        // allocate a mask plane only for these
        if (data->format == MUIV_Pixmap_Format_ARGB32)
        {
            mask = AllocVec(RAWIDTH(data->width) * data->height, MEMF_SHARED|MEMF_CLEAR|MEMF_CHIP);
            data->ditheredMask = mask;
            mPtr = mask;
        }
        else
        {
            data->ditheredMask = NULL;
        }

        for(y = 0; y < data->height; y++)
        {
            LONG x;
            UBYTE bitMask = 0x80;

            for(x = 0; x < data->width; x++)
            {
                UBYTE a, r, g, b;
                ULONG i;
                ULONG bestIndex;
                ULONG bestError;

                // obtain the pixel's A, R, G and B values from the raw data
                switch(data->format)
                {
                    case MUIV_Pixmap_Format_CLUT8:
                        a = (colorMap[dataPtr[0]] >> 24) & 0xff;
                        r = (colorMap[dataPtr[0]] >> 16) & 0xff;
                        g = (colorMap[dataPtr[0]] >>  8) & 0xff;
                        b = (colorMap[dataPtr[0]] >>  0) & 0xff;
                        dataPtr += 1;
                        break;

                    case MUIV_Pixmap_Format_RGB24:
                        a = 0xff;
                        r = dataPtr[0];
                        g = dataPtr[1];
                        b = dataPtr[2];
                        dataPtr += 3;
                        break;

                    case MUIV_Pixmap_Format_ARGB32:
                        a = dataPtr[0];
                        r = dataPtr[1];
                        g = dataPtr[2];
                        b = dataPtr[3];
                        dataPtr += 4;
                        break;

                    default:
                        a = 0x00;
                        r = 0x00;
                        g = 0x00;
                        b = 0x00;
                        break;
                }

                // now calculate the best matching color from the given color map
                bestIndex = 0;
                bestError = 0xffffffffUL;

                for (i = 0; i < 256; i++)
                {
                    LONG dr, dg, db;
                    ULONG error;

                    // calculate the geometric difference to the current color
                    dr = (LONG)((colorMap[i] >> 16) & 0xff) - (LONG)r;
                    dg = (LONG)((colorMap[i] >>  8) & 0xff) - (LONG)g;
                    db = (LONG)((colorMap[i] >>  0) & 0xff) - (LONG)b;
                    error = dr * dr + dg * dg + db * db;

                    if (bestError > error)
                    {
                        // remember this as the best matching color so far
                        bestError = error;
                        bestIndex = i;

                        // bail out if we found an exact match
                        if (error == 0x00000000)
                            break;
                    }
                }

                // put the calculated color number into the destination LUT8 image
                // using the additional pen map
                *ditheredPtr++ = data->ditheredPenMap[bestIndex];

                if (mPtr != NULL)
                {
                    // if we have a mask and the alpha value is >= 0x80 the
                    // pixel is treated as non-transparent
                    if (a >= 0x80)
                        mPtr[x/8] |= bitMask;

                    bitMask >>= 1;
                    if (bitMask == 0x00)
                        bitMask = 0x80;
                }
            }

        // advance the mask pointer by one line
        if (mPtr != NULL)
            mPtr += RAWIDTH(data->width);
        }

        // CyberGraphics cannot blit raw data through a mask, thus we have to
        // take this ugly workaround and take the detour using a bitmap.
        if((data->ditheredBitmap = AllocBitMap(data->width, data->height, 8, BMF_MINPLANES, NULL)) != NULL)
        {
            struct RastPort tempRP;

            InitRastPort(&tempRP);
            tempRP.BitMap = data->ditheredBitmap;

            WritePixelArray(data->ditheredData, 0, 0, data->width, &tempRP, 0, 0, data->width, data->height, RECTFMT_LUT8);
        }
    }
}


static BOOL DecompressRLE(struct IClass *cl, Object *obj, ULONG uncompressedSize)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    BOOL success = FALSE;

    if ((data->uncompressedData = AllocVec(uncompressedSize, MEMF_SHARED)) != NULL)
    {
        LONG rleLen = (LONG)data->compressedSize;
        unsigned char *rleData = (unsigned char *)data->data;
        unsigned char *dest = (unsigned char *)data->uncompressedData;

        while (rleLen != 0)
        {
            unsigned char c;

            c = *rleData++;
            rleLen--;

            if (c & 0x80)
            {
                LONG n = (c & 0x7f) + 2;

                c = *rleData++;
                rleLen--;

                memset(dest, c, n);
                dest += n;
            }
            else
            {
                c++;
                memcpy(dest, rleData, c);
                rleLen -= c;
                rleData += c;
                dest += c;
            }
        }

        success = TRUE;
    }

    return success;
}


static BOOL DecompressBZip2(struct IClass *cl, Object *obj, ULONG uncompressedSize)
{
    BOOL success = FALSE;

    {
        APTR uncompressedData;

        if ((uncompressedData = AllocVec(uncompressedSize, MEMF_SHARED)) != NULL)
        {
            bz_stream bzip2_stream;

            memset(&bzip2_stream, 0, sizeof(bzip2_stream));
            if (BZ2_bzDecompressInit(&bzip2_stream, 0, 1) == BZ_OK)
            {
                struct Pixmap_DATA *data = INST_DATA(cl, obj);
                int err;

                bzip2_stream.next_in = data->data;
                bzip2_stream.avail_in = data->compressedSize;
                bzip2_stream.next_out = uncompressedData;
                bzip2_stream.avail_out = uncompressedSize;

                err = BZ2_bzDecompress(&bzip2_stream);
                if ((err != BZ_OK && err != BZ_STREAM_END) || bzip2_stream.total_out_lo32 != uncompressedSize)
                {
                    FreeVec(uncompressedData);
                    uncompressedData = NULL;
                }
                else
                {
                    data->uncompressedData = uncompressedData;
                    success = TRUE;
                }

                BZ2_bzDecompressEnd(&bzip2_stream);
            }
        }
    }

    return success;
}


static BOOL DecompressImage(struct IClass *cl, Object *obj)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    BOOL success = FALSE;

    if (data->uncompressedData != NULL)
    {
        // the image has been uncompressed before, return immediate success
        success = TRUE;
    }
    else if(data->compression != MUIV_Pixmap_Compression_None && data->data != NULL && data->compressedSize != 0)
    {
        ULONG uncompressedSize;

        switch(data->format)
        {
            case MUIV_Pixmap_Format_CLUT8:
                uncompressedSize = data->width * data->height;
                break;

            case MUIV_Pixmap_Format_RGB24:
                uncompressedSize = data->width * data->height * 3;
                break;

            case MUIV_Pixmap_Format_ARGB32:
                uncompressedSize = data->width * data->height * 4;
                break;

            default:
                uncompressedSize = 0;
                break;
        }

        // uncompress the image data
        switch (data->compression)
        {
            case MUIV_Pixmap_Compression_RLE:
            {
                success = DecompressRLE(cl, obj, uncompressedSize);
            }
            break;

            case MUIV_Pixmap_Compression_BZip2:
            {
                success = DecompressBZip2(cl, obj, uncompressedSize);
            }
            break;

            default:
            {
                success = FALSE;
            }
            break;
        }
    }
    else if (data->data != NULL)
    {
        // nothing to do, return success
        data->uncompressedData = data->data;
        success = TRUE;
    }

    return success;
}


IPTR Pixmap__MUIM_Setup(struct IClass *cl, Object *obj, struct MUIP_Setup *msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    // just try to decompress the image, but don't fail
    DecompressImage(cl, obj);

    if (!DoSuperMethodA(cl, obj, (Msg)msg))
        return FALSE;

    // in case we are to be displayed on a colormapped screen we have to create
    // dithered copies of the images
    if (data->uncompressedData != NULL && (data->screenDepth = GetBitMapAttr(_screen(obj)->RastPort.BitMap, BMA_DEPTH)) <= 8)
    {
        ULONG i;
        const ULONG *colorMap;
        struct TagItem obpTags[] =
        {
            {OBP_Precision, PRECISION_IMAGE },
            {TAG_DONE, 0 }
        };

        // use a user definable colormap or the default color map
        if (data->clut != NULL)
            colorMap = data->clut;
        else
            colorMap = defaultColorMap;

        // allocate all pens
        for(i = 0; i < 256; i++)
            data->ditheredPenMap[i] = ObtainBestPenA(_screen(obj)->ViewPort.ColorMap, ((colorMap[i] >> 16) & 0xff) * 0x01010101UL,
                                                                                      ((colorMap[i] >>  8) & 0xff) * 0x01010101UL,
                                                                                      ((colorMap[i] >>  0) & 0xff) * 0x01010101UL,
                                                                                      obpTags);

        // create a dithered copy of the raw image
        DitherImage(cl, obj);
    }
    else
    {
        data->ditheredData = NULL;
    }

    if (!_isfloating(obj))
    {
        // if there is a change that anything of the parent's imagery may be visible below ourselves
        // then we must make sure that our background is drawn accordingly before we draw ourselves.
        // The background is visible if we either:
        // - have an own alphachannel
        // - are drawn with an additional transparency
        // - have a transparent mask
        if(data->format == MUIV_Pixmap_Format_ARGB32 || data->alpha != 0xffffffffUL || data->ditheredMask != NULL)
            SetSuperAttrs(cl, obj, MUIA_DoubleBuffer, TRUE, MUIA_FillArea, TRUE, TAG_DONE);
        else
            SetSuperAttrs(cl, obj, MUIA_DoubleBuffer, FALSE, MUIA_FillArea, FALSE, TAG_DONE);
    }

    return TRUE;
}


IPTR Pixmap__MUIM_Cleanup(struct IClass *cl, Object *obj, Msg msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    // free the possibly dithered image copy
    if (data->ditheredData != NULL)
    {
        FreeVec(data->ditheredData);
        data->ditheredData = NULL;
    }
    if (data->ditheredMask != NULL)
    {
        FreeVec(data->ditheredMask);
        data->ditheredMask = NULL;
    }
    if (data->ditheredBitmap != NULL)
    {
        FreeBitMap(data->ditheredBitmap);
        data->ditheredBitmap = NULL;
    }

    // release all allocated pens
    if (data->screenDepth <= 8)
    {
        ULONG i;

        for (i = 0; i < 256; i++)
        {
            if(data->ditheredPenMap[i] != -1)
            ReleasePen(_screen(obj)->ViewPort.ColorMap, data->ditheredPenMap[i]);
        }
    }

    return DoSuperMethodA(cl, obj, msg);
}


IPTR Pixmap__MUIM_AskMinMax(struct IClass *cl, Object *obj, struct MUIP_AskMinMax *msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    msg->MinMaxInfo->MinWidth  += data->width;
    msg->MinMaxInfo->MinHeight += data->height;
    msg->MinMaxInfo->DefWidth  += data->width;
    msg->MinMaxInfo->DefHeight += data->height;
    msg->MinMaxInfo->MaxWidth  += data->width;
    msg->MinMaxInfo->MaxHeight += data->height;

    return 0;
}


static void DrawPixmapSection(struct IClass *cl, Object *obj, LONG sx, LONG sy, LONG sw, LONG sh, struct MUI_RenderInfo *mri, LONG dx, LONG dy)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    struct RastPort *rp = mri->mri_RastPort;

    if (data->screenDepth <= 8 && data->ditheredBitmap != NULL)
    {
        // CyberGraphics cannot blit raw data through a mask, thus we have to
        // take this ugly workaround and take the detour using a bitmap.
        if(data->ditheredMask != NULL)
        {
            BltMaskBitMapRastPort(data->ditheredBitmap, sx, sy, rp, dx, dy, sw, sh, (ABC|ABNC|ANBC), data->ditheredMask);
        }
        else
        {
            BltBitMapRastPort(data->ditheredBitmap, sx, sy, rp, dx, dy, sw, sh, (ABC|ABNC));
        }
    }
    else if(data->uncompressedData != NULL)
    {
        switch(data->format)
        {
            case MUIV_Pixmap_Format_CLUT8:
                WriteLUTPixelArray(data->uncompressedData, sx, sy, data->width, rp, data->clut, dx, dy, sw, sh, CTABFMT_XRGB8);
                break;

            case MUIV_Pixmap_Format_RGB24:
                WritePixelArray(data->uncompressedData, sx, sy, data->width*3, rp, dx, dy, sw, sh, RECTFMT_RGB);
                break;

            case MUIV_Pixmap_Format_ARGB32:
                WritePixelArrayAlpha(data->uncompressedData, sx, sy, data->width*4, rp, dx, dy, sw, sh, data->alpha);
                break;
        }
    }
    else
    {
        // just draw a black cross in case we got no valid pixmap
        SetAPen(rp, _pens(obj)[MPEN_TEXT]);
        Move(rp, _mleft(obj), _mtop(obj));
        Draw(rp, _mright(obj), _mbottom(obj));
        Move(rp, _mleft(obj), _mbottom(obj));
        Draw(rp, _mright(obj), _mtop(obj));
    }
}

IPTR Pixmap__MUIM_Draw(struct IClass *cl, Object *obj, struct MUIP_Draw *msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);

    DoSuperMethodA(cl, obj, (Msg)msg);

    if (msg->flags & MADF_DRAWOBJECT)
    {
        if(data->data != NULL)
        {
            int w = MIN(_mwidth (obj), data->width );
            int h = MIN(_mheight(obj), data->height);

            if (w > 0 && h > 0)
            DrawPixmapSection(cl, obj, 0, 0, w, h, muiRenderInfo(obj), _mleft(obj), _mtop(obj));
        }

        if(_isdisabled(obj))
            MUIP_DrawDisablePattern(muiRenderInfo(obj), _left(obj), _top(obj), _width(obj), _height(obj));
    }

    return 0;
}


IPTR Pixmap__OM_SET(struct IClass *cl, Object *obj, struct opSet *msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    BOOL decompress = FALSE;
    BOOL refresh = FALSE;
    const struct TagItem *tags;
    struct TagItem *tag;

    for (tags = msg->ops_AttrList; (tag = NextTagItem(&tags)); )
    {
        switch (tag->ti_Tag)
        {
            case MUIA_Pixmap_Data:           data->data = (APTR)tag->ti_Data; refresh = TRUE; decompress = TRUE; break;
            case MUIA_Pixmap_Format:         data->format = tag->ti_Data; refresh = TRUE; break;
            case MUIA_Pixmap_Width:          data->width  = tag->ti_Data; refresh = TRUE; break;
            case MUIA_Pixmap_Height:         data->height = tag->ti_Data; refresh = TRUE; break;
            case MUIA_Pixmap_CLUT:           data->clut = (APTR)tag->ti_Data; refresh = TRUE; break;
            case MUIA_Pixmap_Alpha:          data->alpha = tag->ti_Data; refresh = TRUE; break;
            case MUIA_Pixmap_Compression:    data->compression = tag->ti_Data; refresh = TRUE; decompress = TRUE; break;
            case MUIA_Pixmap_CompressedSize: data->compressedSize = tag->ti_Data; refresh = TRUE; decompress = TRUE; break;
        }
    }

    if (decompress == TRUE)
    {
        // obtain the new image data
        FreeImage(cl, obj);
        if (DecompressImage(cl, obj) == FALSE)
            return FALSE;
    }

    if (refresh == TRUE)
    {
        MUI_Redraw(obj, MADF_DRAWOBJECT);
    }

    DoSuperMethodA(cl, obj, (Msg)msg);

    // signal success, this is checked by Rawimage.mcc
    return TRUE;
}


IPTR Pixmap__OM_GET(struct IClass *cl, Object *obj, struct opGet *msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    IPTR *store = msg->opg_Storage;

    switch (((struct opGet *)msg)->opg_AttrID)
    {
        case MUIA_Pixmap_Data:
            *store = (ULONG)data->data;
            return TRUE;

        case MUIA_Pixmap_Format:
            *store = data->format;
            return TRUE;

        case MUIA_Pixmap_Width:
            *store = (ULONG)data->width;
            return TRUE;

        case MUIA_Pixmap_Height:
            *store = (ULONG)data->height;
            return TRUE;

        case MUIA_Pixmap_CLUT:
            *store = (ULONG)data->clut;
            return TRUE;

        case MUIA_Pixmap_Alpha:
            *store = data->alpha;
            return TRUE;

        case MUIA_Pixmap_Compression:
            *store = data->compression;
            return TRUE;

        case MUIA_Pixmap_CompressedSize:
            *store = data->compressedSize;
            return TRUE;

        case MUIA_Pixmap_UncompressedData:
            DecompressImage(cl, obj);
            *store = (ULONG)data->uncompressedData;
            return TRUE;
    }

    return DoSuperMethodA(cl, obj, (Msg)msg);
}


IPTR Pixmap__MUIM_Layout(struct IClass *cl, Object *obj,Msg msg)
{
    struct Pixmap_DATA *data = INST_DATA(cl, obj);
    ULONG rc = DoSuperMethodA(cl, obj, (Msg)msg);

    if (data->leftOffset < 0)
        _left(obj) = _right(_parent(obj))-_width(obj)+1+1+data->leftOffset;
    else
        _left(obj) += data->leftOffset;

    if (data->topOffset < 0)
        _top(obj)  = _bottom(_parent(obj))-_height(obj)+1+1+data->topOffset;
    else
        _top(obj)  += data->topOffset;

    return rc;
}


IPTR Pixmap__MUIM_Pixmap_DrawSection(struct IClass *cl, Object *obj, struct MUIP_Pixmap_DrawSection *msg)
{
    BOOL success;

    if ((success = DecompressImage(cl, obj)) == TRUE)
    {
        DrawPixmapSection(cl, obj, msg->sx, msg->sy, msg->sw, msg->sh, msg->mri, msg->dx, msg->dy);
    }

    return 0;
}


#if ZUNE_BUILTIN_PIXMAP
BOOPSI_DISPATCHER(IPTR, Pixmap_Dispatcher, cl, obj, msg)
{
    switch(msg->MethodID)
    {
        case OM_NEW                 : return Pixmap__OM_NEW(cl,obj,(APTR)msg));
        case OM_DISPOSE             : return Pixmap__OM_DISPOSE(cl,obj,(APTR)msg));
        case OM_SET                 : return Pixmap__OM_SET(cl,obj,(APTR)msg));
        case OM_GET                 : return Pixmap__OM_GET(cl,obj,(APTR)msg));
        case MUIM_Draw              : return Pixmap__MUIM_Draw(cl,obj,(APTR)msg));
        case MUIM_Setup             : return Pixmap__MUIM_Setup(cl,obj,(APTR)msg));
        case MUIM_Cleanup           : return Pixmap__MUIM_Cleanup(cl,obj,(APTR)msg));
        case MUIM_AskMinMax         : return Pixmap__MUIM_AskMinMax(cl,obj,(APTR)msg));
        case MUIM_Layout            : return Pixmap__MUIM_Layout(cl,obj,(APTR)msg));
        case MUIM_Pixmap_DrawSection: return Pixmap__MUIM_Pixmap_DrawSection(cl,obj,(APTR)msg));
    }

    return DoSuperMethodA(cl, obj, (APTR)msg);
}
BOOPSI_DISPATCHER_END

const struct __MUIBuiltinClass _MUI_Pixmap_desc = {
    MUIC_Pixmap,
    MUIC_Area,
    sizeof(struct MUI_Pixmap_DATA),
    (void*)Pixmap_Dispatcher
};
#endif
