/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
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
} BmpHandleType;


typedef struct
{
    WORD        bfType;             //  0 ASCII "BM"
    ULONG       bfSize;             //  2 Size in bytes of the file
    WORD        bfReserved1;        //  6 Zero
    WORD        bfReserved2;        //  8 Zero
    ULONG       bfOffBits;          // 10 Byte offset in files where image begins
} FileBitMapHeader __attribute__((packed));    // 14

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
} BitmapInfoHeader __attribute__((packed));    // 40

/* "BM" backwards, due to LE byte order */
#define BITMAP_ID "MB"

/**************************************************************************************************/

static void BMP_Exit(BmpHandleType *bmphandle, LONG errorcode)
{
    D(if (errorcode) bug("bmp.datatype/BMP_Exit() --- IoErr %ld\n", errorcode));
    if (bmphandle->filebuf)
    {
	FreeMem(bmphandle->filebuf, bmphandle->filebufsize);
    }
    if (bmphandle->linebuf)
    {
	FreeMem(bmphandle->linebuf, bmphandle->linebufsize);
    }
    if (bmphandle->codecvars)
    {
	FreeVec(bmphandle->codecvars);
    }
    SetIoErr(errorcode);
}

/**************************************************************************************************/

/* buffered file access, useful for RLE */
BOOL SaveBMP_EmptyBuf(BmpHandleType *bmphandle, long minbytes)
{
    long                bytes, bytestowrite;
    
    bytestowrite = bmphandle->filebufsize - (bmphandle->filebufbytes + minbytes);
    D(bug("bmp.datatype/SaveBMP_EmptyBuf() --- minimum %ld bytes, %ld bytes to write\n", (long)minbytes, (long)bytestowrite));
    bytes = Write(bmphandle->filehandle, bmphandle->filebuf, bytestowrite);
    if ( bytes < bytestowrite )
    {
	D(bug("bmp.datatype/SaveBMP_EmptyBuf() --- writing failed, wrote %ld bytes\n", (long)bytes));
	return FALSE;
    }
    bmphandle->filebufpos = bmphandle->filebuf;
    bmphandle->filebufbytes = bmphandle->filebufsize - minbytes;
    D(bug("bmp.datatype/SaveBMP_EmptyBuf() --- wrote %ld bytes\n", (long)bytes));
    return TRUE;
}

/* buffered file access, useful for RLE */
BOOL LoadBMP_FillBuf(BmpHandleType *bmphandle, long minbytes)
{
    long                i, bytes;
    
    //D(bug("bmp.datatype/LoadBMP_FillBuf() --- minimum %ld bytes of %ld (%ld) bytes\n", (long)minbytes, (long)bmphandle->filebufbytes, (long)(bmphandle->filebufsize-(bmphandle->filebufpos-bmphandle->filebuf)) ));
    if ( bmphandle->filebufbytes >= 0 )
	return TRUE;
    bytes = bmphandle->filebufbytes + minbytes;
    //D(bug("bmp.datatype/LoadBMP_FillBuf() --- %ld bytes requested, %ld bytes left\n", (long)minbytes, (long)bytes));
    if (bytes > 0)
    {
	//D(bug("bmp.datatype/LoadBMP_FillBuf() --- existing %ld old bytes\n", (long)bytes));
	for (i=0; i<bytes; i++)     /* copy existing bytes to start of buffer */
	    bmphandle->filebuf[i] = bmphandle->filebufpos[i];
    }
    bmphandle->filebufpos = bmphandle->filebuf;
    bytes = Read(bmphandle->filehandle, bmphandle->filebuf + bytes, bmphandle->filebufsize - bytes);
    if (bytes < 0 ) bytes = 0;
    bmphandle->filebufbytes += bytes;
    //D(bug("bmp.datatype/LoadBMP_FillBuf() --- read %ld bytes, remaining new %ld bytes\n", (long)bytes, (long)bmphandle->filebufbytes));
    //D(bug("bmp.datatype/LoadBMP_FillBuf() --- >minimum %ld bytes of %ld (%ld) bytes\n", (long)minbytes, (long)bmphandle->filebufbytes, (long)(bmphandle->filebufsize-(bmphandle->filebufpos-bmphandle->filebuf)) ));
    if (bmphandle->filebufbytes >= 0)
	return TRUE;
    return FALSE;
}

static BOOL LoadBMP_Colormap(BmpHandleType *bmphandle, int numcolors,
			    struct ColorRegister *colormap, ULONG *colregs)
{
    unsigned int        i, j;

    if (numcolors && numcolors <= MAXCOLORS)
    {
	j = 0;
	for (i = 0; i < numcolors; i++)
	{
	    if ( (bmphandle->filebufbytes -= 4) < 0 && !LoadBMP_FillBuf(bmphandle, 4) )
	    {
		D(bug("bmp.datatype/LoadBMP_Colormap() --- colormap loading failed\n"));
		return FALSE;
	    }
	    /* BGR0 format for MS Win files, BGR format for OS/2 files */
	    colormap[i].blue = *(bmphandle->filebufpos)++;
	    colormap[i].green = *(bmphandle->filebufpos)++;
	    colormap[i].red = *(bmphandle->filebufpos)++;
	    *(bmphandle->filebufpos)++;
	    colregs[j++] = ((ULONG)colormap[i].red)<<24;
	    colregs[j++] = ((ULONG)colormap[i].green)<<24;
	    colregs[j++] = ((ULONG)colormap[i].blue)<<24;
	    // D(if (i<5) bug("gif r %02lx g %02lx b %02lx\n", colormap[i].red, colormap[i].green, colormap[i].blue));
	}
	D(bug("bmp.datatype/LoadBMP_Colormap() --- %d colors loaded\n", numcolors));
    }
    return TRUE;
}

/**************************************************************************************************/
static BOOL LoadBMP(struct IClass *cl, Object *o)
{
    BmpHandleType           *bmphandle;
    UBYTE                   *filebuf;
    IPTR                    sourcetype;
    ULONG                   bfSize, bfOffBits;
    ULONG                   biSize, biWidth, biHeight, biCompression;
    ULONG                   biClrUsed, biClrImportant;
    UWORD                   biPlanes, biBitCount;
    ULONG                   alignwidth, alignbytes, pixelfmt;
    long                    x, y;
    int                     cont, byte;
    struct BitMapHeader     *bmhd;
    struct ColorRegister    *colormap;
    ULONG                   *colorregs;
    STRPTR                  name;

    D(bug("bmp.datatype/LoadBMP()\n"));

    if( !(bmphandle = AllocMem(sizeof(BmpHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    bmphandle->filebuf = NULL;
    bmphandle->linebuf = NULL;
    bmphandle->codecvars = NULL;
    
    
    if( GetDTAttrs(o,   DTA_SourceType    , (IPTR)&sourcetype ,
			DTA_Handle        , (IPTR)&(bmphandle->filehandle),
			PDTA_BitMapHeader , (IPTR)&bmhd,
			TAG_DONE) != 3 )
    {
	BMP_Exit(bmphandle, ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
    
    if ( sourcetype == DTST_RAM && bmphandle->filehandle == NULL && bmhd )
    {
	D(bug("bmp.datatype/LoadBMP() --- Creating an empty object\n"));
	BMP_Exit(bmphandle, 0);
	return TRUE;
    }
    if ( sourcetype != DTST_FILE || !bmphandle->filehandle || !bmhd )
    {
	D(bug("bmp.datatype/LoadBMP() --- unsupported mode\n"));
	BMP_Exit(bmphandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }
    
    /* initialize buffered file reads */
    bmphandle->filebufbytes = 0;
    bmphandle->filebufsize = FILEBUFSIZE;
    if( !(bmphandle->filebuf = bmphandle->filebufpos = AllocMem(bmphandle->filebufsize, MEMF_ANY)) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    /* load FileBitmapHeader from file, make sure, there are at least 14 bytes in buffer */
    if ( (bmphandle->filebufbytes -= 14) < 0 && !LoadBMP_FillBuf(bmphandle, 14) )
    {
	D(bug("bmp.datatype/LoadBMP() --- filling buffer with header failed\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    filebuf = bmphandle->filebufpos;    /* this makes things easier */
    bmphandle->filebufpos += 14;
    if( filebuf[0] != 'B' && filebuf[1] != 'M' )
    {
	D(bug("bmp.datatype/LoadBMP() --- header type mismatch\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    /* byte-wise access isn't elegant, but it is endianess-safe */
    bfSize = (filebuf[5]<<24) | (filebuf[4]<<16) | (filebuf[3]<<8) | filebuf[2];
    bfOffBits = (filebuf[13]<<24) | (filebuf[12]<<16) | (filebuf[11]<<8) | filebuf[10];
    D(bug("bmp.datatype/LoadBMP() --- bfSize %ld bfOffBits %ld\n", bfSize, bfOffBits));

    /* load BitmapInfoHeader from file, make sure, there are at least 40 bytes in buffer */
    if ( (bmphandle->filebufbytes -= 40) < 0 && !LoadBMP_FillBuf(bmphandle, 40) )
    {
	D(bug("bmp.datatype/LoadBMP() --- filling buffer with header 2 failed\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    filebuf = bmphandle->filebufpos;    /* this makes things easier */
    bmphandle->filebufpos += 40;

    /* get image size attributes */
    biSize = (filebuf[3]<<24) | (filebuf[2]<<16) | (filebuf[1]<<8) | filebuf[0];
    biWidth = (filebuf[7]<<24) | (filebuf[6]<<16) | (filebuf[5]<<8) | filebuf[4];
    biHeight = (filebuf[11]<<24) | (filebuf[10]<<16) | (filebuf[9]<<8) | filebuf[8];
    biPlanes = (filebuf[13]<<8) | filebuf[12];
    biBitCount = (filebuf[15]<<8) | filebuf[14];
    biCompression = (filebuf[19]<<24) | (filebuf[18]<<16) | (filebuf[17]<<8) | filebuf[16];
    biClrUsed = (filebuf[35]<<24) | (filebuf[34]<<16) | (filebuf[33]<<8) | filebuf[32];
    biClrImportant = (filebuf[39]<<24) | (filebuf[38]<<16) | (filebuf[37]<<8) | filebuf[36];
    D(bug("bmp.datatype/LoadBMP() --- BMP-Screen %ld x %ld x %ld, %ld (%ld) colors, compression %ld, type %ld\n",
	  biWidth, biHeight, (long)biBitCount, biClrUsed, biClrImportant, biCompression, biSize));
    if (biSize != 40 || biPlanes != 1 || biCompression != 0)
    {
	D(bug("bmp.datatype/LoadBMP() --- Image format not supported\n"));
	BMP_Exit(bmphandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    /* check color mode */
    pixelfmt = PBPAFMT_LUT8;
    switch (biBitCount)
    {
	case 1:
	    alignwidth = (biWidth + 31) & ~31UL;
	    alignbytes = alignwidth / 8;
	    break;
	case 4:
	    alignwidth = (biWidth + 7) & ~7UL;
	    alignbytes = alignwidth / 2;
	    break;
	case 8:
	    alignwidth = (biWidth + 3) & ~3UL;
	    alignbytes = alignwidth;
	    break;
	case 24:
	    alignbytes = (biWidth + 3) & ~3UL;
	    alignwidth = alignbytes * 3;
	    pixelfmt = PBPAFMT_RGB;
	    break;
	default:
	    D(bug("bmp.datatype/LoadBMP() --- unsupported color depth\n"));
	    BMP_Exit(bmphandle, ERROR_NOT_IMPLEMENTED);
	    return FALSE;
    }
    D(bug("bmp.datatype/LoadBMP() --- align: pixels %ld bytes %ld\n", alignwidth, alignbytes));

    /* set BitMapHeader with image size */
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = biWidth;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = biHeight;
    bmhd->bmh_Depth  = biBitCount;

    /* get empty colormap, then fill in colormap to use*/
    if (biBitCount != 24)
    {
	if( !(GetDTAttrs(o, PDTA_ColorRegisters, (IPTR)&colormap,
			    PDTA_CRegs, (IPTR)&colorregs,
			    TAG_DONE ) == 2) ||
	    !(colormap && colorregs) )
	{
	    D(bug("bmp.datatype/LoadBMP() --- got no colormap\n"));
	    BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
	if( !LoadBMP_Colormap(bmphandle, biClrUsed, colormap, colorregs) )
	{
	    BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
    }
    }
    /* skip offset */
    bfOffBits = bfOffBits - 14 - 40 - biClrUsed*4;
    D(bug("bmp.datatype/LoadBMP() --- remaining offset %ld\n", bfOffBits));
    if ( bfOffBits < 0 ||
	( (bmphandle->filebufbytes -= bfOffBits ) < 0 && !LoadBMP_FillBuf(bmphandle, bfOffBits) ) )
    {
	D(bug("bmp.datatype/LoadBMP() --- cannot skip offset\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    bmphandle->filebufpos += bfOffBits;

    /* Pass attributes to picture.datatype */
    GetDTAttrs( o, DTA_Name, (IPTR)&name, TAG_DONE );
    SetDTAttrs(o, NULL, NULL, PDTA_NumColors, biClrUsed,
			      DTA_NominalHoriz, biWidth,
			      DTA_NominalVert , biHeight,
			      DTA_ObjName     , (IPTR)name,
			      TAG_DONE);

    /* Now decode the picture data into a chunky buffer; and pass it to Bitmap line-by-line */
    bmphandle->linebufsize = bmphandle->linebufbytes = alignwidth;
    if (! (bmphandle->linebuf = bmphandle->linebufpos = AllocMem(bmphandle->linebufsize, MEMF_ANY)) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    //D(bug("bmp.datatype/LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)bmphandle->filebufbytes, (long)(bmphandle->filebufsize-(bmphandle->filebufpos-bmphandle->filebuf)) ));
    cont = 1;
    for (y=biHeight-1; y>=0 && cont; y--)
    {
	int r, g, b;
	
	bmphandle->linebufpos = bmphandle->linebuf;
	if (biBitCount == 24)
	{
	    if ( (bmphandle->filebufbytes -= alignwidth) < 0 && !LoadBMP_FillBuf(bmphandle, alignwidth) )
	    {
		D(bug("bmp.datatype/LoadBMP() --- early end of bitmap data, x %ld y %ld\n", x, y));
		//BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
		//return FALSE;
		cont = 0;
	    }
	    for (x=0; x<alignbytes; x++)
	    {
		b = *(bmphandle->filebufpos)++;
		g = *(bmphandle->filebufpos)++;
		r = *(bmphandle->filebufpos)++;
		*(bmphandle->linebufpos)++ = r;
		*(bmphandle->linebufpos)++ = g;
		*(bmphandle->linebufpos)++ = b;
	    }
	}
	else
	{
	    for (x=0; x<alignbytes; x++)
	    {
		if ( (bmphandle->filebufbytes -= 1) < 0 && !LoadBMP_FillBuf(bmphandle, 1) )
		{
		    D(bug("bmp.datatype/LoadBMP() --- early end of bitmap data, x %ld y %ld\n", x, y));
		    //BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
		    //return FALSE;
		    cont = 0;
		    break;              
		}
		byte = *(bmphandle->filebufpos)++;
		switch (biBitCount)
		{
		    case 1:
			for (b=0; b<8; b++)
			{
			    *(bmphandle->linebufpos)++ = (byte & 0x80) ? 1 : 0;
			    byte <<= 1;
			}
			break;
		    case 4:
			*(bmphandle->linebufpos)++ = (byte & 0xf0) >> 4;
			*(bmphandle->linebufpos)++ = (byte & 0x0f);
			break;
		    case 8:
			*(bmphandle->linebufpos)++ = byte;
			break;
		    case 24:
			*(bmphandle->linebufpos)++ = byte;
			break;
		}
	    }
	}
	if
	(
	    !DoSuperMethod(cl, o,
			   PDTM_WRITEPIXELARRAY,	/* Method_ID */
			   (IPTR)bmphandle->linebuf,	/* PixelData */
			   pixelfmt,			/* PixelFormat */
			   alignwidth,			/* PixelArrayMod (number of bytes per row) */
			   0,				/* Left edge */
			   y,				/* Top edge */
			   biWidth,			/* Width */
			   1				/* Height (here: one line) */
	    )
	)
	{
	    D(bug("bmp.datatype/LoadBMP() --- WRITEPIXELARRAY failed !\n"));
	    BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
    }
    //D(bug("bmp.datatype/LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)bmphandle->filebufbytes, (long)(bmphandle->filebufsize-(bmphandle->filebufpos-bmphandle->filebuf)) ));

    D(bug("bmp.datatype/LoadBMP() --- Normal Exit\n"));
    BMP_Exit(bmphandle, 0);
    return TRUE;
}

/**************************************************************************************************/

static BOOL SaveBMP(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
    BmpHandleType           *bmphandle;
    UBYTE                   *filebuf;
    unsigned int            width, height, widthxheight, numplanes, numcolors;
    struct BitMapHeader     *bmhd;
    struct BitMap           *bm;
    struct RastPort         rp;
    long                    *colorregs;
    int                     i, j, ret;

    D(bug("bmp.datatype/SaveBMP()\n"));

    if( !(bmphandle = AllocMem(sizeof(BmpHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    bmphandle->filebuf = NULL;
    bmphandle->linebuf = NULL;
    bmphandle->codecvars = NULL;

    /* A NULL file handle is a NOP */
    if( !dtw->dtw_FileHandle )
    {
	D(bug("bmp.datatype/SaveBMP() --- empty Filehandle - just testing\n"));
	BMP_Exit(bmphandle, 0);
	return TRUE;
    }
    bmphandle->filehandle = dtw->dtw_FileHandle;

    /* Get BitMap and color palette */
    if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR)&bmhd,
			PDTA_BitMap,       (IPTR)&bm,
			PDTA_CRegs,        (IPTR)&colorregs,
			PDTA_NumColors,    (IPTR)&numcolors,
			TAG_DONE ) != 4UL ||
	!bmhd || !bm || !colorregs || !numcolors)
    {
	D(bug("bmp.datatype/SaveBMP() --- missing attributes\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_NOT_FOUND);
	return FALSE;
    }
#if 0
    /* Check if this is a standard BitMap */
    if( !( GetBitMapAttr(bm, BMA_FLAGS) & BMF_STANDARD ) )
    {
	D(bug("bmp.datatype/SaveBMP() --- wrong BitMap type\n"));
	BMP_Exit(bmphandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
#endif
    /* initialize buffered file reads */
    bmphandle->filebufsize = FILEBUFSIZE;
    bmphandle->filebufbytes = bmphandle->filebufsize;
    if( !(bmphandle->filebuf = bmphandle->filebufpos = AllocMem(bmphandle->filebufsize, MEMF_ANY)) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    /* write BMP 87a header to file, make sure, there are at least 13 bytes in buffer */
    if ( (bmphandle->filebufbytes -= 13) < 0 && !SaveBMP_EmptyBuf(bmphandle, 13) )
    {
	D(bug("bmp.datatype/SaveBMP() --- filling buffer with header failed\n"));
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    filebuf = bmphandle->filebufpos;    /* this makes things easier */
    bmphandle->filebufpos += 13;

    /* set screen descriptor attributes (from BitMapHeader) */
    width = bmhd->bmh_PageWidth;
    height = bmhd->bmh_PageHeight;
    numplanes = bmhd->bmh_Depth - 1;
    numcolors = 1 << (numplanes + 1);
    D(bug("bmp.datatype/SaveBMP() --- BMP-Image %d x %d x %d, cols %d\n", width, height, numplanes+1, numcolors));
    filebuf[6] = width & 0xff;
    filebuf[7] = width >> 8;
    filebuf[8] = height & 0xff;
    filebuf[9] = height >> 8;
    filebuf[10] = 0x80 | ((numplanes & 0x07) << 4) | (numplanes & 0x07) ; /* set numplanes, havecolmap=1 */
    filebuf[11] = 0;    /* this is fillcolor */
    filebuf[12] = 0;    /* this is pixel aspect ratio, 0 means unused */

    /* write screen colormap, we don't use an image colormap */
    for (i = 0; i < numcolors*3; i += 3)
    {
	if ( (bmphandle->filebufbytes -= 3) < 0 && !SaveBMP_EmptyBuf(bmphandle, 3) )
	{
	    BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	    return FALSE;
	}
	*(bmphandle->filebufpos)++ = colorregs[i] >> 24;
	*(bmphandle->filebufpos)++ = colorregs[i+1] >> 24;
	*(bmphandle->filebufpos)++ = colorregs[i+2] >> 24;
    }

    /* write image header, image has same size as screen */
    if ( (bmphandle->filebufbytes -= 10) < 0 && !SaveBMP_EmptyBuf(bmphandle, 10) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    filebuf = bmphandle->filebufpos;    /* this makes things easier */
    bmphandle->filebufpos += 10;
    filebuf[0] = ',';       /* header ID */
    filebuf[1] = filebuf[2] = 0;    /* no left edge */
    filebuf[3] = filebuf[4] = 0;    /* no top edge */
    filebuf[5] = width & 0xff;
    filebuf[6] = width >> 8;
    filebuf[7] = height & 0xff;
    filebuf[8] = height >> 8;
    filebuf[9] = numplanes & 0x07; /* set numplanes, havecolmap=0, interlaced=0 */

    /* Now read the picture data from the bitplanes and write it to a chunky buffer */
    /* For now, we use a full picture pixel buffer, not a single line */
    widthxheight = width*height;
    bmphandle->linebufsize = bmphandle->linebufbytes = widthxheight;
    if (! (bmphandle->linebuf = bmphandle->linebufpos = AllocMem(bmphandle->linebufsize, MEMF_ANY)) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    InitRastPort(&rp);
    rp.BitMap=bm;
    for (j=0; j<height; j++)
    {
	for (i=0; i<width; i++)
	{
	    ret = (UBYTE)ReadPixel(&rp, i, j);  /* very slow, to be changed */
	    *(bmphandle->linebufpos)++ = ret;
	}
    }
    bmphandle->linebufpos = bmphandle->linebuf;

    /* write the chunky buffer to file, after encoding */
    
    /* write end-of-BMP marker */
    if ( !bmphandle->filebufbytes-- && !SaveBMP_EmptyBuf(bmphandle, 1) )
    {
	BMP_Exit(bmphandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }
    *(bmphandle->filebufpos)++ = ';';

    /* flush write buffer to file and exit */
    SaveBMP_EmptyBuf(bmphandle, 0);
    D(bug("bmp.datatype/SaveBMP() --- Normal Exit\n"));
    BMP_Exit(bmphandle, 0);
    return TRUE;
}



/**************************************************************************************************/

IPTR BMP__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;
    
    D(bug("bmp.datatype/DT_Dispatcher: Method OM_NEW\n"));
    
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
    D(bug("bmp.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
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
