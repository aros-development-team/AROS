/*
    Copyright (C) 2022-2025, The AROS Development Team. All rights reserved.
*/

/**********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dostags.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/iffparse.h>
#include <proto/datatypes.h>
#include <proto/tiff.h>

#include <tiffinline.h>

#include <aros/symbolsets.h>

# include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>


#include "debug.h"

#include "methods.h"

ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

/* Dummy functions for the linker */
void abort(void)
{
  exit(1);
}

void exit(int bla)
{
  D(bug("[tiff.datatype] exit\n"));
  abort();
}

/**************************************************************************************************/

static void tiffConvert16to8(UWORD pi, UWORD sspp, ULONG pxfmt, ULONG width, ULONG height, char *src, char *dst)
{
    D(bug("[tiff.datatype] %s(%04x, %04x, %08x, %u, %u, 0x%p, 0x%p)\n", __func__, pi, sspp, pxfmt, width, height, src, dst));

    for (int i = 0; i < (width * height); i++) {
        if (pi < PHOTOMETRIC_RGB)
        {
            UBYTE pixval;

            UWORD *word_ptr = (UWORD *)&src[i << 1];
            pixval = (*word_ptr >> 8) & 0xFF;

            if (pi == PHOTOMETRIC_MINISWHITE) {
                pixval = 255 - pixval;
            }

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = pixval;
                dst[3 * i + 1] = pixval;
                dst[3 * i + 2] = pixval;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = pixval;
                dst[4 * i + 1] = pixval;
                dst[4 * i + 2] = pixval;
                dst[4 * i + 3] = 0xFF;
            }
        }
        else if (sspp == 3)
        {
            UWORD *word_ptr = (UWORD *)&src[(i << 1) * 3];

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = (word_ptr[0] >> 8) & 0xFF;
                dst[3 * i + 1] = (word_ptr[1] >> 8) & 0xFF;
                dst[3 * i + 2] = (word_ptr[2] >> 8) & 0xFF;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = (word_ptr[0] >> 8) & 0xFF;
                dst[4 * i + 1] = (word_ptr[1] >> 8) & 0xFF;
                dst[4 * i + 2] = (word_ptr[2] >> 8) & 0xFF;
                dst[4 * i + 3] = 0xFF;
            }
        }
        else if (sspp == 4)
        {
            UWORD *word_ptr = (UWORD *)&src[(i << 1) * 4];

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = (word_ptr[0] >> 8) & 0xFF;
                dst[3 * i + 1] = (word_ptr[1] >> 8) & 0xFF;
                dst[3 * i + 2] = (word_ptr[2] >> 8) & 0xFF;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = (word_ptr[0] >> 8) & 0xFF;
                dst[4 * i + 1] = (word_ptr[1] >> 8) & 0xFF;
                dst[4 * i + 2] = (word_ptr[2] >> 8) & 0xFF;
                dst[4 * i + 3] = (word_ptr[3] >> 8) & 0xFF;
            }
        }
    }
}


static void tiffConvert32to8(UWORD pi, UWORD sspp, ULONG pxfmt, ULONG width, ULONG height, char *src, char *dst)
{
    D(bug("[tiff.datatype] %s(%04x, %04x, %08x, %u, %u, 0x%p, 0x%p)\n", __func__, pi, sspp, pxfmt, width, height, src, dst));

    for (int i = 0; i < (width * height); i++) {
        if (pi < PHOTOMETRIC_RGB)
        {
            UBYTE pixval;

            ULONG *long_ptr = (ULONG *)&src[i << 1];
            pixval = (*long_ptr >> 8) & 0xFF;

            if (pi == PHOTOMETRIC_MINISWHITE) {
                pixval = 255 - pixval;
            }

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = pixval;
                dst[3 * i + 1] = pixval;
                dst[3 * i + 2] = pixval;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = pixval;
                dst[4 * i + 1] = pixval;
                dst[4 * i + 2] = pixval;
                dst[4 * i + 3] = 0xFF;
            }
        }
        else if (sspp == 3)
        {
            ULONG *long_ptr = (ULONG *)&src[i * 9];

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = (long_ptr[0] >> 8) & 0xFF;
                dst[3 * i + 1] = (long_ptr[1] >> 8) & 0xFF;
                dst[3 * i + 2] = (long_ptr[2] >> 8) & 0xFF;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = (long_ptr[0] >> 8) & 0xFF;
                dst[4 * i + 1] = (long_ptr[1] >> 8) & 0xFF;
                dst[4 * i + 2] = (long_ptr[2] >> 8) & 0xFF;
                dst[4 * i + 3] = 0xFF;
            }
        }
        else if (sspp == 4)
        {
            ULONG *long_ptr = (ULONG *)&src[i * 12];

            if (pxfmt == PBPAFMT_RGB)
            {
                dst[3 * i + 0] = (long_ptr[0] >> 8) & 0xFF;
                dst[3 * i + 1] = (long_ptr[1] >> 8) & 0xFF;
                dst[3 * i + 2] = (long_ptr[2] >> 8) & 0xFF;
            }
            else if (pxfmt == PBPAFMT_RGBA)
            {
                dst[4 * i + 0] = (long_ptr[0] >> 8) & 0xFF;
                dst[4 * i + 1] = (long_ptr[1] >> 8) & 0xFF;
                dst[4 * i + 2] = (long_ptr[2] >> 8) & 0xFF;
                dst[4 * i + 3] = (long_ptr[3] >> 8) & 0xFF;
            }
        }
    }
}

/**************************************************************************************************/

#if !defined(MIN)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

static BOOL LoadTIFF(struct IClass *cl, Object *o)
{
    union {
        struct IFFHandle   *iff;
        BPTR                bptr;
    } filehandle;
    IPTR                    sourcetype;
    struct BitMapHeader     *bmhd;
    TIFF          *tif;
    char tiffFName[1024];

    D(bug("[tiff.datatype] LoadTIFF()\n"));

    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
                        DTA_Handle        , (IPTR)&filehandle,
                        PDTA_BitMapHeader , (IPTR)&bmhd,
                        TAG_DONE) != 3 )
    {
        return FALSE;
    }

    if ( sourcetype == DTST_RAM && filehandle.iff == NULL && bmhd )
    {
        D(bug("[tiff.datatype] LoadTIFF: Creating an empty object\n"));
        return TRUE;
    }
    if ( sourcetype != DTST_FILE || !filehandle.bptr || !bmhd )
    {
        D(bug("[tiff.datatype] LoadTIFF: unsupported mode\n"));

        return FALSE;
    }

    NameFromFH(filehandle.bptr, tiffFName, 1023);
    D(bug("[tiff.datatype] LoadTIFF: opening '%s'\n", tiffFName));

    tif = TIFFOpen(tiffFName, "r");
    if (tif)
    {
        ULONG imageLength = 0, tileLength = 0;
        ULONG imageWidth = 0, tileWidth = 0;
        ULONG RowsPerStrip = 0;
        UWORD BitsPerSample = 0;
        UWORD SamplesPerPixel = 0;
        UWORD PhotometricInterpretation = 0;
        UWORD compression = 0;
        STRPTR name = NULL;
        BOOL isTiled = FALSE;

        D(bug("[tiff.datatype] LoadTIFF: tif @  0x%p\n", tif));

        if (TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression)) {
            if (compression == COMPRESSION_JPEG) {
                TIFFSetField(tif, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
                D(bug("[tiff.datatype] LoadTIFF: JPEG compression detected — using RGB mode\n"));
            } else {
                D(bug("[tiff.datatype] LoadTIFF: TIFF uses compression type: %u (not JPEG)\n", compression));
            }
        } else {
            D(bug("[tiff.datatype] LoadTIFF: Could not read compression tag.\n"));
        }

        TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &BitsPerSample);
        TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP, &RowsPerStrip);  
        TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &SamplesPerPixel);
        TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imageWidth);
        TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &imageLength);  

        TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &PhotometricInterpretation);

        bmhd->bmh_Width  = bmhd->bmh_PageWidth  = imageWidth;
        bmhd->bmh_Height = bmhd->bmh_PageHeight = imageLength;
        if (BitsPerSample <= 8)
            bmhd->bmh_Depth = BitsPerSample * SamplesPerPixel;
        else
            bmhd->bmh_Depth = 8 * SamplesPerPixel;

        D(bug("[tiff.datatype] LoadTIFF: %ux%ux%u (%ux%x)\n", imageWidth, imageLength, bmhd->bmh_Depth, BitsPerSample, SamplesPerPixel));

    /* Pass picture size to picture.datatype */
        GetDTAttrs( o, DTA_Name, (IPTR)&name, TAG_DONE );
        SetDTAttrs(o, NULL, NULL, DTA_NominalHoriz, imageWidth,
                                  DTA_NominalVert , imageLength,
                                  DTA_ObjName     , (IPTR)name,
                                  TAG_DONE);

        ULONG buffersize, x, y;
        UBYTE *buf, *tmp_buf = NULL;
        IPTR pformat;

        if (!TIFFIsTiled(tif))
        {
            tileWidth = imageWidth;
            tileLength = 1;
            buffersize = TIFFScanlineSize(tif);
        }
        else
        {
            TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tileWidth);
            TIFFGetField(tif, TIFFTAG_TILELENGTH, &tileLength);
            buffersize = TIFFTileSize(tif);
            isTiled = TRUE;
        }

        buf = AllocVec(buffersize, 0);

        if (BitsPerSample <= 8)
        {
            BOOL done = FALSE;

            if (SamplesPerPixel == 1)
            {
                if (PhotometricInterpretation == PHOTOMETRIC_MINISWHITE || PhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
                {
                    tmp_buf = AllocVec(tileWidth * tileLength * 3, 0);
                    D(
                    if (BitsPerSample == 1)
                        bug("[tiff.datatype] LoadTIFF[8BPS]: Black & White image\n");
                    else
                        bug("[tiff.datatype] LoadTIFF[8BPS]: %ubit Greyscale image\n", BitsPerSample);
                    )
                    pformat = PBPAFMT_RGB;
                }
                else if (PhotometricInterpretation == PHOTOMETRIC_PALETTE)
                {
                    UWORD                   *red_colormap;
                    UWORD                   *green_colormap;
                    UWORD                   *blue_colormap;

                    D(bug("[tiff.datatype] LoadTIFF[8BPS]: %ubit Palette Mapped\n", BitsPerSample));

                    if (BitsPerSample < 8)
                        tmp_buf = AllocVec(tileWidth * tileLength, 0);

                    if (TIFFGetField(tif, TIFFTAG_COLORMAP, &red_colormap, &green_colormap, &blue_colormap)) {
                        struct ColorRegister    *colorregs = 0;
                        ULONG                   *cregs = 0;
                        SetDTAttrs(o, NULL, NULL, PDTA_NumColors, 1 << BitsPerSample, TAG_DONE);
                        if (GetDTAttrs(o, PDTA_ColorRegisters, (IPTR) &colorregs,
                                          PDTA_CRegs         , (IPTR) &cregs    ,
                                          TAG_DONE                              ) == 2)
                        {
                            for(int i = 0; i < (1 << BitsPerSample); i++)
                            {
                                colorregs->red   = red_colormap[i] >> 8;
                                colorregs->green = green_colormap[i] >> 8;
                                colorregs->blue  = blue_colormap[i] >> 8;

                                *cregs++ = ((ULONG)colorregs->red)   * 0x01010101;
                                *cregs++ = ((ULONG)colorregs->green) * 0x01010101;
                                *cregs++ = ((ULONG)colorregs->blue)  * 0x01010101;

                                colorregs++;
                            }
                            D(bug("[tiff.datatype] LoadTIFF[8BPS]: read %u palette entries\n", 1 << BitsPerSample));
                        } /* if (GetDTAttrs(o, ... */
                    }
                    pformat = PBPAFMT_LUT8;
                }
            }
            else if (SamplesPerPixel == 3)
                pformat = PBPAFMT_RGB;
            else if (SamplesPerPixel == 4)
                pformat = PBPAFMT_RGBA;
            else
            {
                D(bug("[tiff.datatype] LoadTIFF[8BPS]: unhandled SamplesPerPixel\n"));
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength)
            {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth)
                {
                    if (isTiled)
                    {
                        D(bug("[tiff.datatype] LoadTIFF[8BPS]: Tiled read %ux%u @ %u,%u...\n", tileWidth, tileLength, x, y));
                        if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0)
                        {
                            done = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        D(bug("[tiff.datatype] LoadTIFF[8BPS]: Scanline read...\n"));
                        if (TIFFReadScanline(tif, buf, y, 0) != 1)
                        {
                            done = TRUE;
                            break;
                        }
                    }

                    if (!done) {
                        if (tmp_buf)
                        {
                            D(bug("[tiff.datatype] LoadTIFF[8BPS]: converting buffer...\n"));
                            for (int i = 0; i < (tileWidth * tileLength); i++) {
                                UBYTE pixval;

                                if (PhotometricInterpretation == PHOTOMETRIC_PALETTE)
                                {
                                    // Extract pixel from packed bits
                                    ULONG bit_offset = i * BitsPerSample;
                                    UBYTE byte = buf[bit_offset / 8];
                                    UBYTE shift = 8 - BitsPerSample - (bit_offset % 8);
                                    UBYTE mask = (1 << BitsPerSample) - 1;
                                    pixval = (byte >> shift) & mask;
                                    tmp_buf[i] = pixval;
                                }
                                else
                                {
                                    if (BitsPerSample == 8) {
                                        pixval = buf[i];
                                    } else {
                                        // Extract pixel from packed bits
                                        ULONG bit_offset = i * BitsPerSample;
                                        UBYTE byte = buf[bit_offset / 8];
                                        UBYTE shift = 8 - BitsPerSample - (bit_offset % 8);
                                        UBYTE mask = (1 << BitsPerSample) - 1;
                                        pixval = (byte >> shift) & mask;

                                        // Scale to 8 bits
                                        pixval = (pixval * 255) / mask;
                                    }

                                    if (PhotometricInterpretation == PHOTOMETRIC_MINISWHITE) {
                                        pixval = 255 - pixval;
                                    }

                                    // Set RGB
                                    tmp_buf[3 * i + 0] = pixval;
                                    tmp_buf[3 * i + 1] = pixval;
                                    tmp_buf[3 * i + 2] = pixval;
                                }
                            }
                        }
                        D(bug("[tiff.datatype] LoadTIFF[8BPS]: rendering to datatype obj...\n"));
                        if(!DoSuperMethod(cl, o,
                                          PDTM_WRITEPIXELARRAY,                     /* Method_ID */
                                          (IPTR) tmp_buf ? tmp_buf : buf,           /* PixelData */
                                          pformat,                                  /* PixelFormat */
                                          tmp_buf ? (tileWidth >> 3): 0,            /* PixelArrayMod (number of bytes per row) */
                                          x,                                        /* Left edge */
                                          y,                                        /* Top edge */
                                          MIN(tileWidth, bmhd->bmh_Width - x),      /* Width */
                                          MIN(tileLength, bmhd->bmh_Height - y)))    /* Height (here: one line) */
                        {
                            D(bug("[tiff.datatype] LoadTIFF[8BPS]: DT object failed to render\n"));
                            //png_error(png.png_ptr, "Out of memory!");
                            done = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        else if (BitsPerSample == 16)
        {
            BOOL done = FALSE;

            if (SamplesPerPixel == 4)
            {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, 0);
                pformat = PBPAFMT_RGBA;
            }
            else if (SamplesPerPixel < 4)
            {
                tmp_buf = AllocVec(tileWidth * tileLength * 3, 0);
                pformat = PBPAFMT_RGB;
                if (PhotometricInterpretation == PHOTOMETRIC_MINISWHITE || PhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
                {
                    D(bug("[tiff.datatype] LoadTIFF[16BPS]: %ubit Greyscale image\n", BitsPerSample);                    )
                }
            }
            else
            {
                D(bug("[tiff.datatype] LoadTIFF[16BPS]: unhandled SamplesPerPixel\n"));
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength)
            {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth)
                {
                    if (isTiled)
                    {
                        D(bug("[tiff.datatype] LoadTIFF[16BPS]: Tiled read %ux%u @ %u,%u...\n", tileWidth, tileLength, x, y));
                        if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0)
                        {
                            done = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        D(bug("[tiff.datatype] LoadTIFF[16BPS]: Scanline read...\n"));
                        if (TIFFReadScanline(tif, buf, y, 0) != 1)
                        {
                            done = TRUE;
                            break;
                        }
                    }

                    if (!done) {
                        if (tmp_buf)
                        {
                            tiffConvert16to8(PhotometricInterpretation, SamplesPerPixel, pformat, tileWidth, tileLength, buf, tmp_buf);
                        }
                        D(bug("[tiff.datatype] LoadTIFF[16BPS]: rendering to datatype obj...\n"));
                        if(!DoSuperMethod(cl, o,
                                          PDTM_WRITEPIXELARRAY,                     /* Method_ID */
                                          (IPTR) tmp_buf ? tmp_buf : buf,           /* PixelData */
                                          pformat,                                  /* PixelFormat */
                                          tmp_buf ? (tileWidth >> 3): 0,            /* PixelArrayMod (number of bytes per row) */
                                          x,                                        /* Left edge */
                                          y,                                        /* Top edge */
                                          MIN(tileWidth, bmhd->bmh_Width - x),      /* Width */
                                          MIN(tileLength, bmhd->bmh_Height - y)))    /* Height (here: one line) */
                        {
                            D(bug("[tiff.datatype] LoadTIFF[16BPS]: DT object failed to render\n"));
                            //png_error(png.png_ptr, "Out of memory!");
                            done = TRUE;
                            break;
                        }
                    }
                }
            }
        }
        else if (BitsPerSample == 32)
        {
            BOOL done = FALSE;

            if (SamplesPerPixel == 4)
            {
                tmp_buf = AllocVec(tileWidth * tileLength * 4, 0);
                pformat = PBPAFMT_RGBA;
            }
            else if (SamplesPerPixel < 4)
            {
                tmp_buf = AllocVec(tileWidth * tileLength * 3, 0);
                pformat = PBPAFMT_RGB;
                if (PhotometricInterpretation == PHOTOMETRIC_MINISWHITE || PhotometricInterpretation == PHOTOMETRIC_MINISWHITE)
                {
                    D(bug("[tiff.datatype] LoadTIFF[32BPS]: %ubit Greyscale image\n", BitsPerSample);                    )
                }
            }
            else
            {
                D(bug("[tiff.datatype] LoadTIFF[32BPS]: unhandled SamplesPerPixel\n"));
            }

            for(y = 0; !done && y < bmhd->bmh_Height; y += tileLength)
            {
                for(x = 0; !done && x < bmhd->bmh_Width; x += tileWidth)
                {
                    if (isTiled)
                    {
                        D(bug("[tiff.datatype] LoadTIFF[32BPS]: Tiled read %ux%u @ %u,%u...\n", tileWidth, tileLength, x, y));
                        if (TIFFReadTile(tif, buf, x, y, 0, 0) < 0)
                        {
                            done = TRUE;
                            break;
                        }
                    }
                    else
                    {
                        D(bug("[tiff.datatype] LoadTIFF[32BPS]: Scanline read...\n"));
                        if (TIFFReadScanline(tif, buf, y, 0) != 1)
                        {
                            done = TRUE;
                            break;
                        }
                    }

                    if (!done) {
                        if (tmp_buf)
                        {
                            tiffConvert32to8(PhotometricInterpretation, SamplesPerPixel, pformat, tileWidth, tileLength, buf, tmp_buf);
                        }
                        D(bug("[tiff.datatype] LoadTIFF[32BPS]: rendering to datatype obj...\n"));
                        if(!DoSuperMethod(cl, o,
                                          PDTM_WRITEPIXELARRAY,                     /* Method_ID */
                                          (IPTR) tmp_buf ? tmp_buf : buf,           /* PixelData */
                                          pformat,                                  /* PixelFormat */
                                          tmp_buf ? (tileWidth >> 3): 0,            /* PixelArrayMod (number of bytes per row) */
                                          x,                                        /* Left edge */
                                          y,                                        /* Top edge */
                                          MIN(tileWidth, bmhd->bmh_Width - x),      /* Width */
                                          MIN(tileLength, bmhd->bmh_Height - y)))    /* Height (here: one line) */
                        {
                            D(bug("[tiff.datatype] LoadTIFF[32BPS]: DT object failed to render\n"));
                            //png_error(png.png_ptr, "Out of memory!");
                            done = TRUE;
                            break;
                        }
                    }
                }
            }
        }

        D(bug("[tiff.datatype] LoadTIFF: done, cleaning up...\n"));

        FreeVec(tmp_buf);
        FreeVec(buf);

        TIFFClose(tif);

        return TRUE;
    }
    D(bug("[tiff.datatype] LoadTIFF: failed to open tif\n"));

    return FALSE;
}

/**************************************************************************************************/

/**************************************************************************************************/

static BOOL SaveTIFF(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    D(bug("[tiff.datatype] SaveTIFF()\n"));

    return TRUE;
}

/**************************************************************************************************/

IPTR TIFF__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;
    
    D(bug("[tiff.datatype] DT_Dispatcher: Method OM_NEW\n"));

    newobj = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (newobj)
    {
        if (!LoadTIFF(cl, newobj))
        {
            CoerceMethod(cl, newobj, OM_DISPOSE);
            newobj = NULL;
        }
    }
    
    return (IPTR)newobj;
}

/**************************************************************************************************/

IPTR TIFF__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("[tiff.datatype] DT_Dispatcher: Method DTM_WRITE\n"));
    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
        /* Local data format requested */
        return SaveTIFF(cl, o, dtw );
    }
    else
    {
        /* Pass msg to superclass (which writes an IFF ILBM picture)... */
        return DoSuperMethodA( cl, o, (Msg)dtw );
    }
}

/**************************************************************************************************/
