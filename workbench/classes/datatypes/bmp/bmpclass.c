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
    D(if (errorcode) bug("BMP.datatype/BMP_Exit() --- IoErr %ld\n", errorcode));
    if (BMPhandle->filebuf)
    {
	FreeMem(BMPhandle->filebuf, BMPhandle->filebufsize);
    }
    if (BMPhandle->linebuf)
    {
	FreeMem(BMPhandle->linebuf, BMPhandle->linebufsize);
    }
    if (BMPhandle->codecvars)
    {
	FreeVec(BMPhandle->codecvars);
    }
    SetIoErr(errorcode);
}

/**************************************************************************************************/

/* buffered file access */
BOOL SaveBMP_EmptyBuf(BMPHandleType *BMPhandle, long minbytes)
{
    long                bytes, bytestowrite;
    
    bytestowrite = BMPhandle->filebufsize - (BMPhandle->filebufbytes + minbytes);
    D(bug("BMP.datatype/SaveBMP_EmptyBuf() --- minimum %ld bytes, %ld bytes to write\n", (long)minbytes, (long)bytestowrite));
    bytes = Write(BMPhandle->filehandle, BMPhandle->filebuf, bytestowrite);
    if ( bytes < bytestowrite )
    {
	D(bug("BMP.datatype/SaveBMP_EmptyBuf() --- writing failed, wrote %ld bytes\n", (long)bytes));
	return FALSE;
    }
    BMPhandle->filebufpos = BMPhandle->filebuf;
    BMPhandle->filebufbytes = BMPhandle->filebufsize - minbytes;
    D(bug("BMP.datatype/SaveBMP_EmptyBuf() --- wrote %ld bytes\n", (long)bytes));
    return TRUE;
}

/**************************************************************************************************/

/* buffered file access, useful for RLE */
BOOL LoadBMP_FillBuf(BMPHandleType *BMPhandle, long minbytes)
{
    long                i, bytes;
    
    //D(bug("BMP.datatype/LoadBMP_FillBuf() --- minimum %ld bytes of %ld (%ld) bytes\n", (long)minbytes, (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));
    if ( BMPhandle->filebufbytes >= 0 )
	return TRUE;
    bytes = BMPhandle->filebufbytes + minbytes;
    //D(bug("BMP.datatype/LoadBMP_FillBuf() --- %ld bytes requested, %ld bytes left\n", (long)minbytes, (long)bytes));
    if (bytes > 0)
    {
	//D(bug("BMP.datatype/LoadBMP_FillBuf() --- existing %ld old bytes\n", (long)bytes));
	for (i=0; i<bytes; i++)     /* copy existing bytes to start of buffer */
	    BMPhandle->filebuf[i] = BMPhandle->filebufpos[i];
    }
    BMPhandle->filebufpos = BMPhandle->filebuf;
    bytes = Read(BMPhandle->filehandle, BMPhandle->filebuf + bytes, BMPhandle->filebufsize - bytes);
    if (bytes < 0 ) bytes = 0;
    BMPhandle->filebufbytes += bytes;
    //D(bug("BMP.datatype/LoadBMP_FillBuf() --- read %ld bytes, remaining new %ld bytes\n", (long)bytes, (long)BMPhandle->filebufbytes));
    //D(bug("BMP.datatype/LoadBMP_FillBuf() --- >minimum %ld bytes of %ld (%ld) bytes\n", (long)minbytes, (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));
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
		D(bug("BMP.datatype/LoadBMP_Colormap() --- colormap loading failed\n"));
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
	    // D(if (i<5) bug("BMP r %02lx g %02lx b %02lx\n", colormap[i].red, colormap[i].green, colormap[i].blue));
	}
	D(bug("BMP.datatype/LoadBMP_Colormap() --- %d colors loaded\n", numcolors));
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

/****************************************************************************************/

void WriteBytesL_Uint32(LONG val, unsigned char **byteBuffer, int offset)
{
    //Linear Conversion
    unsigned char buffer[4];
    buffer[offset++] = (UBYTE)(val & 0xFF);
    buffer[offset++] = (UBYTE)((val >> 8) & 0xFF);
    buffer[offset++] = (UBYTE)((val >> 16) & 0xFF);
    buffer[offset++] = (UBYTE)((val >> 24) & 0xFF);
    
    memcpy(*byteBuffer, buffer, 4);
}

void WriteBytesL_Uint16(WORD val, unsigned char **byteBuffer, int offset)
{
    //Linear Conversion
    unsigned char buffer[2];
    buffer[offset++] = (UBYTE)(val & 0xFF);
    buffer[offset++] = (UBYTE)(val >> 8);    
    memcpy(*byteBuffer, buffer, 2);
}


void WriteBytesB_Uint32(LONG val, unsigned char **byteBuffer, int offset)
{
    //Byte-Swapping Conversion
    unsigned char buffer[4];
    buffer[offset++] = (UBYTE)((val >> 24) & 0xFF);
    buffer[offset++] = (UBYTE)((val >> 16) & 0xFF);
    buffer[offset++] = (UBYTE)((val >> 8) & 0xFF);
    buffer[offset++] = (UBYTE)(val & 0xFF);
    memcpy(*byteBuffer, buffer, 4);
}

void WriteBytesB_Uint16(WORD val, unsigned char **byteBuffer, int offset)
{
    //Byte-Swapping Conversion
    unsigned char buffer[2];
    buffer[offset++] = (UBYTE)(val >> 8);
    buffer[offset++] = (UBYTE)(val & 0xFF);
    memcpy(*byteBuffer, buffer, 2);
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
    ULONG                   alignwidth, alignedwidth, alignbytes, alignedbytes, pixelfmt;
    long                    x, y;
    int                     cont, byte;
    int 		    numcolors;
    struct BitMapHeader     *bmhd;
    struct ColorRegister    *colormap;
    ULONG                   *colorregs;
    STRPTR                  name;

    D(bug("BMP.datatype/LoadBMP()\n"));

    if( !(BMPhandle = AllocMem(sizeof(BMPHandleType), MEMF_ANY)) )
    {
	SetIoErr(ERROR_NO_FREE_STORE);
	return FALSE;
    }
    BMPhandle->filebuf = NULL;
    BMPhandle->linebuf = NULL;
    BMPhandle->codecvars = NULL;
    
    
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
	D(bug("BMP.datatype/LoadBMP() --- Creating an empty object\n"));
	BMP_Exit(BMPhandle, 0);
	return TRUE;
    }
    if ( sourcetype != DTST_FILE || !BMPhandle->filehandle || !bmhd )
    {
	D(bug("BMP.datatype/LoadBMP() --- unsupported mode\n"));
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
	D(bug("BMP.datatype/LoadBMP() --- filling buffer with header failed\n"));
	BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    filebuf = BMPhandle->filebufpos;    /* this makes things easier */
    BMPhandle->filebufpos += 14;
    if( filebuf[0] != 'B' && filebuf[1] != 'M' )
    {
	D(bug("BMP.datatype/LoadBMP() --- header type mismatch\n"));
	BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
	return FALSE;
    }
    /* byte-wise access isn't elegant, but it is endianess-safe */
    bfSize = (filebuf[5]<<24) | (filebuf[4]<<16) | (filebuf[3]<<8) | filebuf[2];
    bfOffBits = (filebuf[13]<<24) | (filebuf[12]<<16) | (filebuf[11]<<8) | filebuf[10];
    D(bug("BMP.datatype/LoadBMP() --- bfSize %ld bfOffBits %ld\n", bfSize, bfOffBits));

    /* load BitmapInfoHeader from file, make sure, there are at least 40 bytes in buffer */
    if ( (BMPhandle->filebufbytes -= 40) < 0 && !LoadBMP_FillBuf(BMPhandle, 40) )
    {
	D(bug("BMP.datatype/LoadBMP() --- filling buffer with header 2 failed\n"));
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
    D(bug("BMP.datatype/LoadBMP() --- BMP-Screen %ld x %ld x %ld, %ld (%ld) colors, compression %ld, type %ld\n",
	  biWidth, biHeight, (long)biBitCount, biClrUsed, biClrImportant, biCompression, biSize));
    if (biSize != 40 || biPlanes != 1 || biCompression != 0)
    {
	D(bug("BMP.datatype/LoadBMP() --- Image format not supported\n"));
	BMP_Exit(BMPhandle, ERROR_NOT_IMPLEMENTED);
	return FALSE;
    }

    /* check color mode */
    numcolors = 0; 
    pixelfmt = PBPAFMT_LUT8;
    switch (biBitCount)
    {
	case 1:
	    alignwidth = (biWidth + 31) & ~31UL;
	    alignbytes = alignwidth / 8;
            numcolors = 2;
	    break;
	case 4:
	    alignwidth = (biWidth + 7) & ~7UL;
	    alignbytes = alignwidth / 2;
            numcolors = 16;
	    break;
	case 8:
	    alignwidth = (biWidth + 3) & ~3UL;          
	    alignbytes = alignwidth;
            numcolors = 256;
	    break;
	case 24:
	    alignbytes = (biWidth + 3) & ~3UL;
	    alignwidth = alignbytes * 3; //2376 
        //Or Use Correction Factor
        alignedwidth = ((biWidth * 3) + 3) & ~3UL; //Not Modulus 4
		//alignedbytes = ((int)(alignwidth / 3)) + 3;
        if(alignwidth != alignedwidth)
        {
            alignwidth = alignedwidth;
            //alignbytes = alignedbytes;
        }
	    pixelfmt = PBPAFMT_RGB;
	    break;
      case 32:
	    alignbytes = biWidth;
	    alignwidth = alignbytes * 4;
	    //pixelfmt = PBPAFMT_RGBA;
            pixelfmt = PBPAFMT_ARGB;
	    break;
	default:
	    D(bug("BMP.datatype/LoadBMP() --- unsupported color depth\n"));
	    BMP_Exit(BMPhandle, ERROR_NOT_IMPLEMENTED);
	    return FALSE;
    }
    D(bug("BMP.datatype/LoadBMP() --- align: pixels %ld bytes %ld\n", alignwidth, alignbytes));

    /* set BitMapHeader with image size */
    bmhd->bmh_Width  = bmhd->bmh_PageWidth  = biWidth;
    bmhd->bmh_Height = bmhd->bmh_PageHeight = biHeight;
    bmhd->bmh_Depth  = biBitCount;

    /* get empty colormap, then fill in colormap to use*/
    //if (biBitCount != 24)
	if (biBitCount <= 8)
    {
	if( !(GetDTAttrs(o, PDTA_ColorRegisters, (IPTR)&colormap,
			    PDTA_CRegs, (IPTR)&colorregs,
			    TAG_DONE ) == 2) ||
	    !(colormap && colorregs) )
	{
	    D(bug("BMP.datatype/LoadBMP() --- got no colormap\n"));
	    BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
	if( !LoadBMP_Colormap(BMPhandle, numcolors, colormap, colorregs) )
	{
	    BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
    }
    }
    
    /* skip offset */
    bfOffBits = bfOffBits - 14 - 40 - numcolors*4;
    D(bug("BMP.datatype/LoadBMP() --- remaining offset %ld\n", bfOffBits));
    if ( bfOffBits < 0 ||
	( (BMPhandle->filebufbytes -= bfOffBits ) < 0 && !LoadBMP_FillBuf(BMPhandle, bfOffBits) ) )
    {
	D(bug("BMP.datatype/LoadBMP() --- cannot skip offset\n"));
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
    UBYTE linebuf[alignwidth];
    BMPhandle->linebufsize = BMPhandle->linebufbytes = alignwidth;
    if (! (BMPhandle->linebuf = BMPhandle->linebufpos = AllocMem(BMPhandle->linebufsize, MEMF_ANY)) )
    {
	BMP_Exit(BMPhandle, ERROR_NO_FREE_STORE);
	return FALSE;
    }

    //D(bug("BMP.datatype/LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));
    cont = 1;
    for (y=biHeight-1; y>=0 && cont; y--)
    {
	    //int r, g, b;
        int r, g, b, a;
	
	BMPhandle->linebufpos = BMPhandle->linebuf;
	if (biBitCount == 24)
	{
	    if ( (BMPhandle->filebufbytes -= alignwidth) < 0 && !LoadBMP_FillBuf(BMPhandle, alignwidth) )
	    {
		D(bug("BMP.datatype/LoadBMP() --- early end of bitmap data, x %ld y %ld\n", x, y));
		//BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
		//return FALSE;
		cont = 0;
	    }
	    for (x=0; x<alignbytes; x++)
	    {
		b = *(BMPhandle->filebufpos)++;
		g = *(BMPhandle->filebufpos)++;
		r = *(BMPhandle->filebufpos)++;
		*(BMPhandle->linebufpos)++ = r;
		*(BMPhandle->linebufpos)++ = g;
		*(BMPhandle->linebufpos)++ = b;
	    }
	}
      else if (biBitCount == 32)
	{	    
	    if ( (BMPhandle->filebufbytes -= alignwidth) < 0 && !LoadBMP_FillBuf(BMPhandle, alignwidth) )
	    {
		D(bug("BMP.datatype/LoadBMP() --- early end of bitmap data, x %ld y %ld\n", x, y));
		//BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
		//return FALSE;
		cont = 0;
	    }
	    for (x=0; x<alignbytes; x++)
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
	else
	{
	    for (x=0; x<alignbytes; x++)
	    {
		if ( (BMPhandle->filebufbytes -= 1) < 0 && !LoadBMP_FillBuf(BMPhandle, 1) )
		{
		    D(bug("BMP.datatype/LoadBMP() --- early end of bitmap data, x %ld y %ld\n", x, y));
		    //BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
		    //return FALSE;
		    cont = 0;
		    break;              
		}
		byte = *(BMPhandle->filebufpos)++;
		switch (biBitCount)
		{
		    case 1:
			for (b=0; b<8; b++)
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
		    //case 24:
			//*(BMPhandle->linebufpos)++ = byte;
			//break;
            	//case 32:
			//*(BMPhandle->linebufpos)++ = byte;
			//break;
		}
	    }
	}
	if
	(
	    !DoSuperMethod(cl, o,
			   PDTM_WRITEPIXELARRAY,	/* Method_ID */
			   (IPTR)BMPhandle->linebuf,	/* PixelData */
			   pixelfmt,			/* PixelFormat */
			   alignwidth,			/* PixelArrayMod (number of bytes per row) */
			   0,				/* Left edge */
			   y,				/* Top edge */
			   biWidth,			/* Width */
			   1				/* Height (here: one line) */
	    )
	)
	{
	    D(bug("BMP.datatype/LoadBMP() --- WRITEPIXELARRAY failed !\n"));
	    BMP_Exit(BMPhandle, ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
	}
    }
    //D(bug("BMP.datatype/LoadBMP() --- bytes of %ld (%ld) bytes\n", (long)BMPhandle->filebufbytes, (long)(BMPhandle->filebufsize-(BMPhandle->filebufpos-BMPhandle->filebuf)) ));

    D(bug("BMP.datatype/LoadBMP() --- Normal Exit\n"));
    BMP_Exit(BMPhandle, 0);
    return TRUE;
}

/**************************************************************************************************/

static BOOL SaveBMP(struct IClass *cl, Object *o, struct dtWrite *dtw )
{
	BPTR		    filehandle;
	unsigned int    width, height, depth, y, yoffset, imageIdx;
    	int             numcolors;
	UBYTE		    *linebuf;
    	UBYTE           tempRGB;
	struct BitMapHeader     *bmhd;
    	struct ColorRegister    *colormap;
    	unsigned char *buffer = "";
	long                    *colorregs;
    	ULONG                   alignwidth, alignbytes, pixelfmt;

	D(bug("bmp.datatype/SaveBMP()\n"));

	/* A NULL file handle is a NOP */
	if( !dtw->dtw_FileHandle )
	{
		D(bug("bmp.datatype/SaveBMP() --- empty Filehandle - just testing\n"));
		return TRUE;
	}
	filehandle = dtw->dtw_FileHandle;

	/* Get BitMapHeader, colormap & colorregs */
	if( GetDTAttrs( o,  PDTA_BitMapHeader, (IPTR) &bmhd,
                PDTA_ColorRegisters,     (IPTR)&colormap,
				PDTA_CRegs,        (IPTR) &colorregs,
				TAG_DONE ) != 3UL ||
			!bmhd || !colormap || !colorregs )
	{
		D(bug("bmp.datatype/SaveBMP() --- missing attributes\n"));
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		return FALSE;
	}

	/* Get width, height, depth */
	width = bmhd->bmh_Width;
	height = bmhd->bmh_Height;
	depth = bmhd->bmh_Depth;
	D(bug("BMP.datatype/SaveBMP() --- Picture size %d x %d (x %d bits)\n", width, height, depth));
    
    
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
	    D(bug("BMP.datatype/LoadBMP() --- unsupported color depth\n"));
	    SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	    return FALSE;
    }
    D(bug("BMP.datatype/LoadBMP() --- align: pixels %ld bytes %ld\n", alignwidth, alignbytes));
	
    
    //** Only Save 8bit or 24bit for Testing **//
	//if(( depth != 8 ) || ( depth != 24 ))
      //if( ( depth != 8 ) && ( depth != 24 ) && ( depth != 32 ) )
      //if( depth != 24 )
      //if( depth != 8 )
	//{
		//D(bug("BMP.datatype/SaveBMP() --- color depth %d, can save only depths of 24\n", depth));
		//SetIoErr(ERROR_OBJECT_WRONG_TYPE);
		//return FALSE;
	//}
	//D(bug("BMP.datatype/SaveBMP() --- Picture size %d x %d (x %d bit)\n", width, height, depth));
    
    
    //** Test for Modulus & Set Padding Values. **//	
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
	
	
    //** Set Values for FileHeader & Infoheader **//    
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
    
        /** Write file signature, fileheader & infoheader to file **/
		
        /** Write fileheader data to file **/
        char *sig = "BM"; 
        memcpy(buffer, sig, 2);          
        FWrite( filehandle, buffer, 2, 1 );
    
            
        WriteBytesL_Uint32(bfsize, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint16(bfReserved, &buffer, 0);
        FWrite( filehandle, buffer, 2, 1 );
        WriteBytesL_Uint16(bfReserved, &buffer, 0);
        FWrite( filehandle, buffer, 2, 1 );
        WriteBytesL_Uint32(bfOffBits, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );    
    	        
        /** Write infoheader data to file **/
        WriteBytesL_Uint32(biSize, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biWidth, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biHeight, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        
        WriteBytesL_Uint16(biPlanes, &buffer, 0);
        FWrite( filehandle, buffer, 2, 1 );
        WriteBytesL_Uint16(biBitCount, &buffer, 0);
        FWrite( filehandle, buffer, 2, 1 );
        
        WriteBytesL_Uint32(biCompression, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biSizeImage, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biXPelsPerMeter, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biYPelsPerMeter, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biClrUsed, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );
        WriteBytesL_Uint32(biClrImportant, &buffer, 0);
        FWrite( filehandle, buffer, 4, 1 );   
    
    
        /** If biBitCount == 8 Convert ColorMap to RGBQuads & Write to file **/
    	if( depth == 8 )
		{        	
        	/** Write RGBQuads Color Data to file **/
            numcolors = 256;
			WriteBMP_Colormap(filehandle, numcolors, colormap);
    	}

    
	/* Now read the picture data line by line and write it to a chunky buffer */        
    if( !(linebuf = AllocVec(alignwidth, MEMF_ANY)) )	
	{
		SetIoErr(ERROR_NO_FREE_STORE);
		return FALSE;
	}
	D(bug("bmp.datatype/PPM_Save() --- copying picture with READPIXELARRAY\n"));

    
    //** Read ScanLine Data from Bottom to Top for BMP **// 
	yoffset = height - 1;    
	for (y=0; y<height; y++)
	{
		if(!DoSuperMethod(cl, o,
					PDTM_READPIXELARRAY,	/* Method_ID */
					(IPTR)linebuf,		/* PixelData */
					pixelfmt,		/* PixelFormat */
					alignwidth,			/* PixelArrayMod (number of bytes per row) */
					0,			/* Left edge */
					yoffset,			/* Top edge */
					width,			/* Width */
					1))			/* Height */
		{
			D(bug("bmp.datatype/SaveBMP() --- READPIXELARRAY line %d failed !\n", y));
			FreeVec(linebuf);
			SetIoErr(ERROR_OBJECT_WRONG_TYPE);
			return FALSE;
		}
        
        if(depth == 24)
		{
        	//** Reverse 'RGB' to 'BGR' for BMP file for 24bit **//
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
        	//** Reverse 'RGBA' to 'BGRA' for BMP file for 32bit **//
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
			D(bug("bmp.datatype/SaveBMP() --- writing picture data line %d failed !\n", y));
			FreeVec(linebuf);
			return FALSE;
		}		
        yoffset--;
	}

	D(bug("bmp.datatype/SaveBMP() --- Normal Exit\n"));
	FreeVec(linebuf);
	SetIoErr(0);
	return TRUE;
}

/**************************************************************************************************/

IPTR BMP__OM_NEW(Class *cl, Object *o, Msg msg)
{
    Object *newobj;
    
    D(bug("BMP.datatype/DT_Dispatcher: Method OM_NEW\n"));
    
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
    D(bug("BMP.datatype/DT_Dispatcher: Method DTM_WRITE\n"));
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
