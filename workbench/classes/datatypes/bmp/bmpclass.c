/*
    Copyright © 1995-2020, The AROS Development Team. All rights reserved.
    $Id: BMPclass.c 30902 2009-03-14 13:38:20Z mazze $ 2020-01-04 miker.
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

#include <aros/symbolsets.h>

#include "debug.h"

#include "methods.h"

/* Open superclass */
ADD2LIBS("datatypes/picture.datatype", 0, struct Library *, PictureBase);

/**************************************************************************************************/

#define FILEBUFSIZE 65536
#define MAXCOLORS   256

typedef struct {
    struct IFFHandle    *filehandle;

    UBYTE               *filebuf;
    UBYTE               *filebufpos;
    long                filebufbytes;
    long                filebufsize;
    UBYTE               *linebuf;
    UBYTE               *linebufpos;
    long                linebufbytes;
    long                linebufsize;
    
    APTR                codecvars;
} BMPHandleType;

typedef struct
{
    UBYTE rgbBlue; 
    UBYTE rgbGreen;
    UBYTE rgbRed;
    UBYTE rgbZero;    
} __attribute__((packed)) RGBQUAD;    

typedef struct
{
    WORD        bfType;             //  0 ASCII "BM"
    ULONG       bfSize;             //  2 Size in bytes of the file
    WORD        bfReserved1;        //  6 Zero
    WORD        bfReserved2;        //  8 Zero
    ULONG       bfOffBits;          // 10 Byte offset in files where image begins
} __attribute__((packed)) FileBitMapHeader;    // 14

typedef struct
{
    ULONG       biSize;             //  0 Size of this header, 40 bytes
    LONG        biWidth;            //  4 Image width in pixels
    LONG        biHeight;           //  8 Image height in pixels
    WORD        biPlanes;           // 12 Number of image planes, must be 1
    WORD        biBitCount;         // 14 Bits per pixel, 1, 4, 8, 24, or 32
    ULONG       biCompression;      // 16 Compression type, below
    ULONG       biSizeImage;        // 20 Size in bytes of compressed image, or zero
    LONG        biXPelsPerMeter;    // 24 Horizontal resolution, in pixels/meter
    LONG        biYPelsPerMeter;    // 28 Vertical resolution, in pixels/meter
    ULONG       biClrUsed;          // 32 Number of colors used, below
    ULONG       biClrImportant;     // 36 Number of "important" colors
} __attribute__((packed)) BitmapInfoHeader;    // 40

/* "BM" backwards, due to LE byte order */
#define BITMAP_ID "MB"

/**************************************************************************************************/

static void BMP_Exit(BMPHandleType *BMPhandle, LONG errorcode)
{
    D(if (errorcode) bug("[bmp.datatype] %s: IoErr %ld\n", __func__, errorcode));

    if (BMPhandle->filebuf)
    {
        FreeMem(BMPhandle->filebuf, BMPhandle->filebufsize);
        BMPhandle->filebuf = NULL;
    }
    if (BMPhandle->linebuf)
    {
        FreeMem(BMPhandle->linebuf, BMPhandle->linebufsize);
        BMPhandle->linebuf = NULL;
    }
    FreeVec(BMPhandle->codecvars);
    BMPhandle->codecvars = NULL;

    SetIoErr(errorcode);
}

/**************************************************************************************************/

/* buffered file access */
BOOL SaveBMP_EmptyBuf(BMPHandleType *BMPhandle, long minbytes)
{
    long                bytes, bytestowrite;
    
    bytestowrite = BMPhandle->filebufsize - (BMPhandle->filebufbytes + minbytes);
    D(bug("[bmp.datatype] %s: minimum %ld bytes, %ld bytes to write\n", __func__, (long)minbytes, (long)bytestowrite));
    bytes = Write(BMPhandle->filehandle, BMPhandle->filebuf, bytestowrite);
    if ( bytes < bytestowrite )
    {
        D(bug("[bmp.datatype] %s: writing failed, wrote %ld bytes\n", __func__, (long)bytes));
        return FALSE;
    }
    BMPhandle->filebufpos = BMPhandle->filebuf;
    BMPhandle->filebufbytes = BMPhandle->filebufsize - minbytes;
    D(bug("[bmp.datatype] %s: wrote %ld bytes\n", __func__, (long)bytes));
    return TRUE;
}

/**************************************************************************************************/

/* buffered file access, useful for RLE */
BOOL LoadBMP_FillBuf(BMPHandleType *BMPhandle, long minbytes)
{
    long                i, bytes;
    
    D(bug("[bmp.datatype] %s: minimum %ld bytes of %ld (%ld) bytes\n", __func__, (long)minbytes, (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf))));

    if ( BMPhandle->filebufbytes >= 0 )
        return TRUE;
    bytes = BMPhandle->filebufbytes + minbytes;
    D(bug("[bmp.datatype] %s: %ld bytes requested, %ld bytes left\n", __func__, (long)minbytes, (long)bytes));
    if (bytes > 0)
    {
        D(bug("[bmp.datatype] %s: existing %ld old bytes\n", __func__, (long)bytes));
        for (i=0; i<bytes; i++)     /* copy existing bytes to start of buffer */
            BMPhandle->filebuf[i] = BMPhandle->filebufpos[i];
    }
    BMPhandle->filebufpos = BMPhandle->filebuf;
    bytes = Read(BMPhandle->filehandle, BMPhandle->filebuf + bytes, BMPhandle->filebufsize - bytes);
    if (bytes < 0 ) bytes = 0;
    BMPhandle->filebufbytes += bytes;
    D(
        bug("[bmp.datatype] %s: read %ld bytes, remaining new %ld bytes\n", __func__, (long)bytes, (long)BMPhandle->filebufbytes);
        bug("[bmp.datatype] %s: minimum %ld bytes of %ld (%ld) bytes\n", __func__, (long)minbytes, (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) );
    )
    if (BMPhandle->filebufbytes >= 0)
        return TRUE;
    return FALSE;
}

/**************************************************************************************************/

static BOOL LoadBMP_Colormap(BMPHandleType *BMPhandle, int numcolors,
                            struct ColorRegister *colormap, ULONG *colregs)
{
    unsigned int        i, j;

    if (numcolors && numcolors <= MAXCOLORS)
    {
        j = 0;
        for (i = 0; i < numcolors; i++)
        {
            if ( (BMPhandle->filebufbytes -= 4) < 0 && !LoadBMP_FillBuf(BMPhandle, 4) )
            {
                D(bug("[bmp.datatype] %s: colormap loading failed\n", __func__));
                return FALSE;
            }
            /* BGR0 format for MS Win files, BGR format for OS/2 files */
            colormap[i].blue = *(BMPhandle->filebufpos)++;
            colormap[i].green = *(BMPhandle->filebufpos)++;
            colormap[i].red = *(BMPhandle->filebufpos)++;
            BMPhandle->filebufpos++;
            colregs[j++] = ((ULONG)colormap[i].red)<<24;
            colregs[j++] = ((ULONG)colormap[i].green)<<24;
            colregs[j++] = ((ULONG)colormap[i].blue)<<24;
            D(if (i<5) bug("[bmp.datatype] %s:  r %02lx g %02lx b %02lx\n", __func__, colormap[i].red, colormap[i].green, colormap[i].blue));
        }
        D(bug("[bmp.datatype] %s: %d colors loaded\n", __func__, numcolors));
    }
    return TRUE;
}

/**************************************************************************************************/

static BOOL WriteBMP_Colormap(BPTR filehandle, int numcolors,
                            struct ColorRegister *colormap)
{
    //Convert ColorMap to RGBQuads 'BGR0'
    unsigned int len, i;
    len = (numcolors * 4);
    RGBQUAD palette[256];
    for(i = 0; i < 256; ++i)
    {
        palette[i].rgbBlue = colormap[i].blue;
        palette[i].rgbGreen = colormap[i].green;
        palette[i].rgbRed = colormap[i].red;
        palette[i].rgbZero = 255;
    }
                
    FWrite( filehandle, palette, len, 1 );
}

/**************************************************************************************************/

static BOOL LoadBMP(struct IClass *cl, Object *o)
{
    BMPHandleType           *BMPhandle;
    UBYTE                   *filebuf;
    IPTR                    sourcetype;
    ULONG                   bfSize, bfOffBits;
    ULONG                   biSize, biWidth, biHeight, biCompression;
    ULONG                   biClrUsed, biClrImportant;
    UWORD                   biPlanes, biBitCount;
    ULONG                   alignwidth, alignbytes, alignedbytes, pixelfmt;
    long                    x, y;
    int                     cont, byte;
    int 		    numcolors;
    struct BitMapHeader     *bmhd;
    struct ColorRegister    *colormap;
    ULONG                   *colorregs;
    STRPTR                  name;

    D(bug("[bmp.datatype] %s()\n", __func__));

    if( !(BMPhandle = AllocMem(sizeof(BMPHandleType), MEMF_CLEAR)) )
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return FALSE;
    }
    
    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
                        DTA_Handle        , (IPTR)&(BMPhandle->filehandle),
                        PDTA_BitMapHeader , (IPTR)&bmhd,
                        TAG_DONE) != 3 )
    {
        BMP_Exit(BMPhandle, ERROR_OBJECT_NOT_FOUND);
        return FALSE;
    }
    
    if ( sourcetype == DTST_RAM && BMPhandle->filehandle == NULL && bmhd )
    {
        D(bug("[bmp.datatype] %s: Creating an empty object\n", __func__));
        BMP_Exit(BMPhandle, 0);
        return TRUE;
    }
    if ( sourcetype != DTST_FILE || !BMPhandle->filehandle || !bmhd )
    {
        D(bug("[bmp.datatype] %s: unsupported mode\n", __func__));
        BMP_Exit(BMPhandle, ERROR_NOT_IMPLEMENTED);
        return FALSE;
    }
    
    /* initialize buffered file reads */
    BMPhandle->filebufbytes = 0;
    BMPhandle->filebufsize = FILEBUFSIZE;
    if( !(BMPhandle->filebuf = BMPhandle->filebufpos = AllocMem(BMPhandle->filebufsize, MEMF_ANY)) )
    {
        BMP_Exit(BMPhandle, ERROR_NO_FREE_STORE);
        return FALSE;
    }

    /* load FileBitmapHeader from file, make sure, there are at least 14 bytes in buffer */
    if ( (BMPhandle->filebufbytes -= 14) < 0 && !LoadBMP_FillBuf(BMPhandle, 14) )
    {
        D(bug("[bmp.datatype] %s: filling buffer with header failed\n", __func__));
        BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    filebuf = BMPhandle->filebufpos;    /* this makes things easier */
    BMPhandle->filebufpos += 14;
    if( filebuf[0] != 'B' && filebuf[1] != 'M' )
    {
        D(bug("[bmp.datatype] %s: header type mismatch\n", __func__));
        BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    /* byte-wise access isn't elegant, but it is endianess-safe */
    bfSize = (filebuf[5]<<24) | (filebuf[4]<<16) | (filebuf[3]<<8) | filebuf[2];
    bfOffBits = (filebuf[13]<<24) | (filebuf[12]<<16) | (filebuf[11]<<8) | filebuf[10];
    D(bug("[bmp.datatype] %s: bfSize %ld bfOffBits %ld\n", __func__, bfSize, bfOffBits));

//TODO: the first ULONG should contain the DIB header size
#if (1)
    /* load BitmapInfoHeader from file, make sure, there are at least 40 bytes in buffer */
    if ( (BMPhandle->filebufbytes -= 40) < 0 && !LoadBMP_FillBuf(BMPhandle, 40) )
    {
        D(bug("[bmp.datatype] %s: filling buffer with header 2 failed\n", __func__));
        BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    filebuf = BMPhandle->filebufpos;    /* this makes things easier */
    BMPhandle->filebufpos += 40;

    /* get image size attributes */
    biSize = (filebuf[3]<<24) | (filebuf[2]<<16) | (filebuf[1]<<8) | filebuf[0];
    biWidth = (filebuf[7]<<24) | (filebuf[6]<<16) | (filebuf[5]<<8) | filebuf[4];
    biHeight = (filebuf[11]<<24) | (filebuf[10]<<16) | (filebuf[9]<<8) | filebuf[8];
    biPlanes = (filebuf[13]<<8) | filebuf[12];
    biBitCount = (filebuf[15]<<8) | filebuf[14];
    biCompression = (filebuf[19]<<24) | (filebuf[18]<<16) | (filebuf[17]<<8) | filebuf[16];
    biClrUsed = (filebuf[35]<<24) | (filebuf[34]<<16) | (filebuf[33]<<8) | filebuf[32];
    biClrImportant = (filebuf[39]<<24) | (filebuf[38]<<16) | (filebuf[37]<<8) | filebuf[36];
    D(bug("[bmp.datatype] %s: BMP-Screen %ld x %ld x %ld, %ld (%ld) colors, compression %ld, type %ld\n", __func__,
          biWidth, biHeight, (long)biBitCount, biClrUsed, biClrImportant, biCompression, biSize));
    if (biSize != 40 || biPlanes != 1 || biCompression != 0)
    {
        D(bug("[bmp.datatype] %s: Image format not supported\n", __func__));
        BMP_Exit(BMPhandle, ERROR_NOT_IMPLEMENTED);
        return FALSE;
    }
#endif
    
    /* check color mode */
    numcolors = 0; 
    pixelfmt = PBPAFMT_LUT8;
    switch (biBitCount)
    {
    case 1:
        alignwidth = (biWidth + 31) & ~31UL;
        alignbytes = alignwidth >> 3;
        numcolors = 2;
        break;

    case 4:
        alignwidth = (biWidth + 7) & ~7UL;
        alignbytes = alignwidth >> 1;
        numcolors = 16;
        break;

    case 8:
        alignwidth = (biWidth + 3) & ~3UL;
        alignbytes = alignwidth;
        numcolors = 256;
        break;

    case 16:
        alignwidth = (biWidth + 1) & ~1UL;
        alignbytes = ((biWidth << 1) + 3) & ~3UL;
        pixelfmt = PBPAFMT_RGB;
        break;

    case 24:
        alignwidth = (biWidth + 3) & ~3UL;
        alignbytes = ((biWidth << 1) + biWidth + 3) & ~3UL;
        pixelfmt = PBPAFMT_RGB;
        break;

  case 32:
        alignwidth = biWidth;
        alignbytes = biWidth << 2;
        pixelfmt = PBPAFMT_ARGB;
        break;

    default:
        D(bug("[bmp.datatype] %s: unsupported color depth\n", __func__));
        BMP_Exit(BMPhandle, ERROR_NOT_IMPLEMENTED);
        return FALSE;
    }
    D(bug("[bmp.datatype] %s: align: pixels %ld bytes %ld\n", __func__, alignwidth, alignbytes));

    /* set BitMapHeader with image size */
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = biWidth;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = biHeight;

    /* get empty colormap, then fill in colormap to use*/
    if ((bmhd->bmh_Depth = biBitCount) <= 8)
    {
        if( !(GetDTAttrs(o, PDTA_ColorRegisters, (IPTR)&colormap,
            PDTA_CRegs, (IPTR)&colorregs,
            TAG_DONE ) == 2) ||
            !(colormap && colorregs) )
        {
            D(bug("[bmp.datatype] %s: got no colormap\n", __func__));
            BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
            return FALSE;
        }
        D(bug("[bmp.datatype] %s: colormap @ 0x%p, colorregs @ 0x%p\n", __func__, colormap, colorregs));
        if( !LoadBMP_Colormap(BMPhandle, numcolors, colormap, colorregs) )
        {
            BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
            return FALSE;
        }
    }
    else if (bmhd->bmh_Depth == 16)
        bmhd->bmh_Depth = 24;

    /* skip offset */
    bfOffBits = bfOffBits - 14 - 40 - numcolors*4;
    D(bug("[bmp.datatype] %s: remaining offset %ld\n", __func__, bfOffBits));
    if ( bfOffBits < 0 ||
        ( (BMPhandle->filebufbytes -= bfOffBits ) < 0 && !LoadBMP_FillBuf(BMPhandle, bfOffBits) ) )
    {
        D(bug("[bmp.datatype] %s: cannot skip offset\n", __func__));
        BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    BMPhandle->filebufpos += bfOffBits;

    /* Pass attributes to picture.datatype */
    GetDTAttrs( o, DTA_Name, (IPTR)&name, TAG_DONE );
    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, numcolors,
                              DTA_NominalHoriz, biWidth,
                              DTA_NominalVert , biHeight,
                              DTA_ObjName     , (IPTR)name,
                              TAG_DONE);

    /* Now decode the picture data into a chunky buffer; and pass it to Bitmap line-by-line */
    ULONG linebufwidth = alignbytes;
    if (biBitCount == 16)
        linebufwidth += (alignbytes >> 1);

    D(bug("[bmp.datatype] %s: linebufwidth = %u, alignbytes = %u\n", __func__, linebufwidth, alignbytes));

    BMPhandle->linebufsize = BMPhandle->linebufbytes = linebufwidth;
    if (! (BMPhandle->linebuf = BMPhandle->linebufpos = AllocMem(BMPhandle->linebufsize, MEMF_ANY)) )
    {
        BMP_Exit(BMPhandle, ERROR_NO_FREE_STORE);
        return FALSE;
    }

    //D(bug("[bmp.datatype] LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));
    cont = 1;
    for (y=biHeight-1; y>=0 && cont; y--)
    {
        BMPhandle->linebufpos = BMPhandle->linebuf;
        if (biBitCount == 32)
        {
            UBYTE r, g, b, a;

            if ( (BMPhandle->filebufbytes -= alignbytes) < 0 && !LoadBMP_FillBuf(BMPhandle, alignbytes) )
            {
                D(bug("[bmp.datatype] %s: early end of bitmap data, x %ld y %ld\n", __func__, x, y));
                //BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
                //return FALSE;
                cont = 0;
            }
            for (x=0; x < alignwidth; x++)
            {
                b = *(BMPhandle->filebufpos)++;
                g = *(BMPhandle->filebufpos)++;
                r = *(BMPhandle->filebufpos)++;
                a = *(BMPhandle->filebufpos)++;
                *(BMPhandle->linebufpos)++ = a;            
                *(BMPhandle->linebufpos)++ = r;
                *(BMPhandle->linebufpos)++ = g;
                *(BMPhandle->linebufpos)++ = b;
                //*(BMPhandle->linebufpos)++ = a;            		                       
            }
        }
        else if (biBitCount == 24)
        {
            UBYTE r, g, b;

            if ( (BMPhandle->filebufbytes -= alignbytes) < 0 && !LoadBMP_FillBuf(BMPhandle, alignbytes) )
            {
                D(bug("[bmp.datatype] %s: early end of bitmap data, x %ld y %ld\n", __func__, x, y));
                //BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
                //return FALSE;
                cont = 0;
            }

            for (x=0; x < biWidth; x++)
            {
                b = *(BMPhandle->filebufpos)++;
                g = *(BMPhandle->filebufpos)++;
                r = *(BMPhandle->filebufpos)++;
                *(BMPhandle->linebufpos)++ = r;
                *(BMPhandle->linebufpos)++ = g;
                *(BMPhandle->linebufpos)++ = b;
            }
            x *= 3;
            for (; x < alignbytes; x++)
            {
                b = *(BMPhandle->filebufpos)++;
                *(BMPhandle->linebufpos)++ = b;
            }
        }
        else if (biBitCount == 16)
        {
            UWORD pixel;

            if ( (BMPhandle->filebufbytes -= alignbytes) < 0 && !LoadBMP_FillBuf(BMPhandle, alignbytes) )
            {
                D(bug("[bmp.datatype] %s: early end of bitmap data, x %ld y %ld\n", __func__, x, y));
                cont = 0;
            }

            for (x=0; x < biWidth; x++)
            {
                pixel = *(BMPhandle->filebufpos)++;
                pixel |= (*(BMPhandle->filebufpos)++) << 8;
                /* Use bit replication to give a fuller range of colours
                   (so e.g. pure white is possible) */
                *(BMPhandle->linebufpos) = (pixel & 0x7C00) >> 7;
                *(BMPhandle->linebufpos) |= *BMPhandle->linebufpos >> 5;
                BMPhandle->linebufpos++;
                *(BMPhandle->linebufpos) = (pixel & 0x03E0) >> 2;
                *(BMPhandle->linebufpos) |= *BMPhandle->linebufpos >> 5;
                BMPhandle->linebufpos++;
                *(BMPhandle->linebufpos) = (pixel & 0x001F) << 3;
                *(BMPhandle->linebufpos) |= *BMPhandle->linebufpos >> 5;
                BMPhandle->linebufpos++;
            }
            x <<= 1;
            for (; x < alignbytes; x++)
            {
                pixel = *(BMPhandle->filebufpos)++;
                *(BMPhandle->linebufpos)++ = (pixel & 0xFF);
            }
        }
        else
        {
            UBYTE bit;

            for (x=0; x<alignbytes; x++)
            {
                if ( (BMPhandle->filebufbytes -= 1) < 0 && !LoadBMP_FillBuf(BMPhandle, 1) )
                {
                    D(bug("[bmp.datatype] %s: early end of bitmap data, x %ld y %ld\n", __func__, x, y));
                    //BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
                    //return FALSE;
                    cont = 0;
                    break;              
                }
                byte = *(BMPhandle->filebufpos)++;
                switch (biBitCount)
                {
                    case 1:
                        for (bit=0; bit<8; bit++)
                        {
                            *(BMPhandle->linebufpos)++ = (byte & 0x80) ? 1 : 0;
                            byte <<= 1;
                        }
                        break;
                    case 4:
                        *(BMPhandle->linebufpos)++ = (byte & 0xf0) >> 4;
                        *(BMPhandle->linebufpos)++ = (byte & 0x0f);
                        break;
                    case 8:
                        *(BMPhandle->linebufpos)++ = byte;
                        break;
                }
            }
        }

        if
        (
            !DoSuperMethod(cl, o,
                   PDTM_WRITEPIXELARRAY,	/* Method_ID */
                   (IPTR)BMPhandle->linebuf,	/* PixelData */
                   pixelfmt,			/* PixelFormat */
                   linebufwidth,		/* PixelArrayMod (number of bytes per row) */
                   0,				/* Left edge */
                   y,				/* Top edge */
                   biWidth,			/* Width */
                   1				/* Height (here: one line) */
            )
        )
        {
            D(bug("[bmp.datatype] %s: WRITEPIXELARRAY failed !\n", __func__));
            BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
            return FALSE;
        }
    }
    //D(bug("[bmp.datatype] LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));

    D(bug("[bmp.datatype] %s: Normal Exit\n", __func__));
    BMP_Exit(BMPhandle, 0);
    return TRUE;
}

/**************************************************************************************************/

static BOOL SaveBMP(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    BPTR		        filehandle;
    unsigned int                width, height, depth, y, yoffset, imageIdx;
    int                         numcolors;
    UBYTE		        *linebuf;
    UBYTE                       tempRGB;
    struct BitMapHeader         *bmhd;
    struct ColorRegister        *colormap;
    long                        *colorregs;
    ULONG                       alignwidth, alignbytes, pixelfmt;
    ULONG 			ulbuff;
    UWORD			uwbuff;

    D(bug("[bmp.datatype] %s()\n", __func__));

    /* A NULL file handle is a NOP */
    if( !dtw->dtw_FileHandle )
    {
        D(bug("[bmp.datatype] %s: empty Filehandle - just testing\n", __func__));
        return TRUE;
    }
    filehandle = dtw->dtw_FileHandle;

    /* Get BitMapHeader, colormap & colorregs */
    if( GetDTAttrs( o, PDTA_BitMapHeader, (IPTR) &bmhd,
            PDTA_ColorRegisters, (IPTR)&colormap,
            PDTA_CRegs, (IPTR) &colorregs,
            TAG_DONE ) != 3UL ||
            !bmhd || !colormap || !colorregs )
    {
        D(bug("[bmp.datatype] %s: missing attributes\n", __func__));
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }

    /* Get width, height, depth */
    width = bmhd->bmh_Width;
    height = bmhd->bmh_Height;
    depth = bmhd->bmh_Depth;
    D(bug("[bmp.datatype] %s: Picture size %d x %d (x %d bits)\n", __func__, width, height, depth));
    
    /* Check bit depth to add padding & to determine pixel format */
    pixelfmt = PBPAFMT_LUT8; //Default bit depth is set to 8.	
    switch (depth)
    {
    case 1:
        alignwidth = (width + 31) & ~31UL;
        alignbytes = alignwidth / 8;
        break;
    case 4:
        alignwidth = (width + 7) & ~7UL;
        alignbytes = alignwidth / 2;
        break;
    case 8:
        alignwidth = (width + 3) & ~3UL;
        alignbytes = alignwidth;
        break;
    case 24:
        alignbytes = (width + 3) & ~3UL;
        alignwidth = alignbytes * 3;
        pixelfmt = PBPAFMT_RGB;
        break;
    case 32:
        alignbytes = (width + 3) & ~3UL;
        //alignbytes = width;
        alignwidth = alignbytes * 4;
        pixelfmt = PBPAFMT_RGBA; //PBPAFMT_ARGB;
        break;
    default:
        D(bug("[bmp.datatype] %s: unsupported color depth\n", __func__));
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    D(bug("[bmp.datatype] %s: align: pixels %ld bytes %ld\n", __func__, alignwidth, alignbytes));
        
#if (0)
    /* Only Save 8bit or 24bit for Testing */
    if(( depth != 8 ) || ( depth != 24 ))
    if( ( depth != 8 ) && ( depth != 24 ) && ( depth != 32 ) )
    if( depth != 24 )
    if( depth != 8 )
    {
        D(bug("[bmp.datatype] %s: color depth %d, can save only depths of 24\n", __func__, depth));
        SetIoErr(ERROR_OBJECT_WRONG_TYPE);
        return FALSE;
    }
    D(bug("[bmp.datatype] %s: Picture size %d x %d (x %d bit)\n", __func__, width, height, depth));
#endif

    /* Test for Modulus & Set Padding Values. */	
    LONG pixelElements = (depth / 8);
    //Is it Modulus 4??
    if((width * pixelElements)%4 == 0)
    {
        alignwidth = (width * pixelElements); //Modulus 4
        alignbytes = width;
    }
    else
    {
        alignwidth = ((width * pixelElements) + 3) & ~3UL; //Not Modulus 4
        alignbytes = (alignwidth / pixelElements);
    }
        
        
    /* Set Values for FileHeader & Infoheader */    
    LONG bfOffBits = 14 + 40; //14+40=54
    LONG biSizeImage = alignwidth * height;
    if (depth == 8)
    {
        biSizeImage = (alignwidth * height) + 1024; //Include Palette Size
        bfOffBits = 14 + 40 + 1024; //14+40+1024=1078 //Palette Size
    }
    LONG bfsize = bfOffBits + biSizeImage; 
    WORD bfReserved = 0;    
    ULONG       biSize = 40;             //  0 Size of this header, 40 bytes
    LONG        biWidth = (LONG)width;            //  4 Image width in pixels
    LONG        biHeight = (LONG)height;           //  8 Image height in pixels
    WORD        biPlanes = 1;           // 12 Number of image planes, must be 1
    WORD        biBitCount = (WORD)depth;         // 14 Bits per pixel, 1, 4, 8, 24, or 32
    ULONG       biCompression = 0;      // 16 Compression type, below
    //ULONG       biSizeImage = ;        // 20 Size in bytes of compressed image, or zero
    LONG        biXPelsPerMeter = 0;    // 24 Horizontal resolution, in pixels/meter
    LONG        biYPelsPerMeter = 0;    // 28 Vertical resolution, in pixels/meter
    ULONG       biClrUsed = 0;          // 32 Number of colors used, below
    ULONG       biClrImportant = 0;     // 36 Number of "important" colors
    if (depth == 8)
    {
        biClrUsed = 256;
        biClrImportant = 256;
    }
    
    /* Write file signature, fileheader & infoheader to file */

    /* Write fileheader data to file */
    char *sig = "BM"; 
    memcpy(&ulbuff, sig, 2);          
    FWrite( filehandle, &ulbuff, 2, 1 );

    ulbuff = AROS_LONG2LE(bfsize);
    FWrite( filehandle, &ulbuff, 4, 1 );
    uwbuff = AROS_WORD2LE(bfReserved);
    FWrite( filehandle, &uwbuff, 2, 1 );
    uwbuff = AROS_WORD2LE(bfReserved);
    FWrite( filehandle, &uwbuff, 2, 1 );
    ulbuff = AROS_LONG2LE(bfOffBits);
    FWrite( filehandle, &ulbuff, 4, 1 );    
            
    /* Write infoheader data to file */
    ulbuff = AROS_LONG2LE(biSize);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biWidth);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biHeight);
    FWrite( filehandle, &ulbuff, 4, 1 );
    
    uwbuff = AROS_WORD2LE(biPlanes);
    FWrite( filehandle, &uwbuff, 2, 1 );
    uwbuff = AROS_WORD2LE(biBitCount);
    FWrite( filehandle, &uwbuff, 2, 1 );
    
    ulbuff = AROS_LONG2LE(biCompression);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biSizeImage);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biXPelsPerMeter);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biYPelsPerMeter);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biClrUsed);
    FWrite( filehandle, &ulbuff, 4, 1 );
    ulbuff = AROS_LONG2LE(biClrImportant);
    FWrite( filehandle, &ulbuff, 4, 1 );   
    
    /* If biBitCount == 8 Convert ColorMap to RGBQuads & Write to file */
    if( depth == 8 )
    {        	
        /* Write RGBQuads Color Data to file */
        numcolors = 256;
        WriteBMP_Colormap(filehandle, numcolors, colormap);
    }

    /* Now read the picture data line by line and write it to a chunky buffer */        
    if( !(linebuf = AllocVec(alignwidth, MEMF_ANY)) )	
    {
        SetIoErr(ERROR_NO_FREE_STORE);
        return FALSE;
    }
    D(bug("[bmp.datatype] %s: copying picture with READPIXELARRAY\n", __func__));

    /* Read ScanLine Data from Bottom to Top for BMP */ 
    yoffset = height - 1;    
    for (y=0; y<height; y++)
    {
        if(!DoSuperMethod(cl, o,
                PDTM_READPIXELARRAY,    /* Method_ID */
                (IPTR)linebuf,          /* PixelData */
                pixelfmt,               /* PixelFormat */
                alignwidth,             /* PixelArrayMod (number of bytes per row) */
                0,                      /* Left edge */
                yoffset,                /* Top edge */
                width,                  /* Width */
                1))                     /* Height */
        {
                D(bug("[bmp.datatype] %s: READPIXELARRAY line %d failed !\n", __func__, y));
                FreeVec(linebuf);
                SetIoErr(ERROR_OBJECT_WRONG_TYPE);
                return FALSE;
        }
    
        if(depth == 24)
        {
            /* Reverse 'RGB' to 'BGR' for BMP file for 24bit */
            for (imageIdx = 0;imageIdx < (width * 3);imageIdx+=3)
            {
                //linebuf = line buffer
                tempRGB = linebuf[imageIdx];
                linebuf[imageIdx] = linebuf[imageIdx + 2];
                linebuf[imageIdx + 2] = tempRGB;
            }
        }
        if(depth == 32)
        {
            /* Reverse 'RGBA' to 'BGRA' for BMP file for 32bit */
            for (imageIdx = 0;imageIdx < (width * 4);imageIdx+=4)
            {
                //linebuf = line buffer
                tempRGB = linebuf[imageIdx];
                linebuf[imageIdx] = linebuf[imageIdx + 2];
                linebuf[imageIdx + 2] = tempRGB;
                //Skip linebuf[imageIdx + 3] Alpha Value.
            }
        }

        if( FWrite( filehandle, linebuf, alignwidth, 1 ) != 1 )
        {
            D(bug("[bmp.datatype] %s: writing picture data line %d failed !\n", __func__, y));
            FreeVec(linebuf);
            return FALSE;
        }		
        yoffset--;
    }

    D(bug("[bmp.datatype] %s: Normal Exit\n", __func__));

    FreeVec(linebuf);
    SetIoErr(0);

    return TRUE;
}

/**************************************************************************************************/

IPTR BMP__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;
    
    D(bug("[bmp.datatype] %s()\n", __func__));
    
    newobj = (Object *)DoSuperMethodA(cl, o, msg);
    if (newobj)
    {
        if (!LoadBMP(cl, newobj))
        {
            CoerceMethod(cl, newobj, OM_DISPOSE);
            newobj = NULL;
        }
    }

    return (IPTR)newobj;
}

/**************************************************************************************************/

IPTR BMP__DTM_WRITE(Class *cl, Object *o, struct dtWrite *dtw)
{
    D(bug("[bmp.datatype] %s()\n", __func__));

    if( (dtw -> dtw_Mode) == DTWM_RAW )
    {
        /* Local data format requested */
        return SaveBMP(cl, o, dtw );
    }
    else
    {
        /* Pass msg to superclass (which writes an IFF ILBM picture)... */
        return DoSuperMethodA( cl, o, (Msg)dtw );
    }
}
