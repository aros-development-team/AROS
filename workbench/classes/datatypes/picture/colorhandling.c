/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfxbase.h>
#include <graphics/rpattr.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/cghooks.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/utility.h>
#include <proto/cybergraphics.h>

#include "debug.h"
#include "pictureclass.h"
#include "colorhandling.h"

static void ScaleLineSimple( UBYTE *srcxptr, UBYTE *destxptr, ULONG destwidth, UWORD srcpixelbytes, ULONG xscale );
static BOOL ScaleArraySimple( struct Picture_Data *pd, struct RastPort rp );
static UBYTE * AllocLineBuffer( long width, long height, int pixelbytes );
static void CopyColTable( struct Picture_Data *pd );
static BOOL RemapCM2CM( struct Picture_Data *pd );
static BOOL RemapTC2CM( struct Picture_Data *pd );
static int HistSort( const void *HistEntry1, const void *HistEntry2 );
static void RemapPens( struct Picture_Data *pd, int NumColors, int DestNumColors );

/**************************************************************************************************/
/*
static const UBYTE defcolmap[] =
{
    0,118,14,117,116,15,115,4,233,232,234,230,229,228,251,162,
    30,70,29,127,114,28,79,19,149,227,246,226,225,224,223,245,
    244,222,221,220,219,161,218,217,27,113,22,108,111,23,110,31,
    216,215,214,213,212,134,132,143,5,109,13,106,107,17,120,7,
    50,105,37,104,103,63,68,61,211,210,243,145,138,148,254,208,
    58,67,2,80,81,44,82,45,242,152,207,206,205,204,235,153,
    202,201,200,199,198,197,196,195,57,88,62,84,85,56,77,55,
    167,255,168,169,130,248,170,165,54,66,53,86,87,52,69,51,
    60,78,49,89,65,46,123,34,141,172,173,159,136,174,175,176,
    41,90,47,122,124,35,121,43,177,135,163,236,178,142,179,249,
    180,181,146,241,237,182,157,183,48,95,42,93,71,3,72,38,
    184,158,185,186,187,155,189,190,36,94,59,119,96,40,73,39,
    6,97,10,75,99,12,100,8,137,191,253,192,166,252,193,140,
    18,101,26,102,98,32,126,25,194,164,156,188,239,147,250,131,
    139,171,238,128,247,150,203,151,24,64,20,125,76,21,92,33,
    209,129,240,144,160,133,154,231,9,91,16,83,112,11,74,1
};
*/
static const UBYTE defcolmap[] =
{
    0,66,8,68,64,10,65,2,136,130,150,142,153,148,149,151,
    16,72,17,76,70,18,79,20,141,131,144,162,160,168,169,164,
    177,147,174,170,143,176,180,178,21,77,22,83,80,23,82,24,
    140,173,181,175,182,184,172,179,3,84,9,85,86,13,88,5,
    36,89,38,91,92,35,90,37,137,157,156,190,165,185,191,188,
    39,93,40,75,81,33,96,43,135,128,146,171,194,196,195,193,
    163,167,129,186,192,197,187,189,44,87,32,94,95,46,97,47,
    183,199,198,138,201,200,203,202,45,98,49,99,100,48,102,50,
    34,103,54,71,67,41,74,51,158,208,210,212,211,213,209,205,
    42,107,55,108,110,52,109,53,152,155,159,207,216,218,219,221,
    139,161,154,166,206,204,133,215,56,69,58,114,73,59,78,57,
    145,217,214,220,222,223,224,225,60,105,61,113,101,63,106,62,
    4,115,12,112,104,14,117,6,228,132,227,229,230,231,234,236,
    25,119,26,121,118,27,120,28,235,237,239,238,240,241,242,243,
    232,244,247,226,249,250,253,252,19,124,29,125,116,30,111,31,
    134,233,246,245,255,254,248,251,7,122,11,123,126,15,127,1
};

/**************************************************************************************************/

BOOL ConvertTC2TC( struct Picture_Data *pd )
{
    struct RastPort DestRP;
    long success;

    D(bug("picture.datatype/ConvertTC2TC: TrueColor source/dest, no remapping required, PixelFormat %ld\n", pd->SrcPixelFormat));
    CopyColTable( pd );
    InitRastPort( &DestRP );
    DestRP.BitMap = pd->DestBM;
    if( !pd->Scale )
    {
	success = WritePixelArray(  pd->SrcBuffer,		// src buffer
				    0,				// src x
				    0,				// src y
				    pd->SrcWidthBytes,		// src mod
				    &DestRP,			// rastport
				    0,				// dest x
				    0,				// dest y
				    pd->SrcWidth,		// width
				    pd->SrcHeight,		// height
				    pd->SrcPixelFormat);	// src format
    }
    else
    {
	success = ScaleArraySimple( pd, DestRP );
    }
#ifdef __AROS__
    DeinitRastPort( &DestRP );
#endif
    return success ? TRUE : FALSE;
}

BOOL ConvertCM2TC( struct Picture_Data *pd )
{
    struct RastPort DestRP;
    long success;

    D(bug("picture.datatype/ConvertCM2TC: Colormapped source, TrueColor dest, no remapping required\n"));
    CopyColTable( pd );
    InitRastPort( &DestRP );
    DestRP.BitMap = pd->DestBM;
    if( !pd->Scale )
    {
	success = WriteLUTPixelArray(   pd->SrcBuffer,	// src buffer
					0,			// src x
					0,			// src y
					pd->SrcWidthBytes,	// src mod
					&DestRP,		// rastport
					pd->ColTableXRGB,	// coltable
					0,			// dest x
					0,			// dest y
					pd->SrcWidth,	// width
					pd->SrcHeight,	// height
					CTABFMT_XRGB8 );	// coltable format
    }
    else
    {
	success = ScaleArraySimple( pd, DestRP );
    }
#ifdef __AROS__
    DeinitRastPort( &DestRP );
#endif
    return success ? TRUE : FALSE;
}

BOOL ConvertCM2CM( struct Picture_Data *pd )
{
    struct RastPort DestRP;
    BOOL success;

    if( pd->Remap )
    {
	D(bug("picture.datatype/ConvertCM2CM: Colormapped source, Colormapped dest, remapping pens\n"));
	success = RemapCM2CM( pd );
    }
    else
    {
        D(bug("picture.datatype/ConvertCM2CM: Colormapped source, Colormapped dest, remapping disabled\n"));
	CopyColTable( pd );
	InitRastPort( &DestRP );
	DestRP.BitMap = pd->DestBM;
	WriteChunkyPixels( &DestRP,
			0,
			0,
			pd->SrcWidth-1,
			pd->SrcHeight-1,
			pd->SrcBuffer,
			pd->SrcWidthBytes );
#ifdef __AROS__
	DeinitRastPort( &DestRP );
#endif
	success = TRUE;
    }
    return success;
}

BOOL ConvertTC2CM( struct Picture_Data *pd )
{
    BOOL success;

    D(bug("picture.datatype/ConvertCM2CM: Truecolor source, Colormapped dest, decreasing depth and remapping\n"));
    success = RemapTC2CM( pd );
    return success;
}

/**************************************************************************************************/

static void ScaleLineSimple( UBYTE *srcxptr, UBYTE *destxptr, ULONG destwidth, UWORD srcpixelbytes, ULONG xscale )
{
    unsigned int srcx, destx, srcxinc;
    ULONG srcxpos;
    UBYTE r=0, g=0, b=0, a;
    UBYTE *srcxpixel;

    a = 0;
    srcx = 0;
    srcxinc = 1;
    srcxpos = 0;
    if( srcpixelbytes == 1 )
	{
	destx = destwidth;
	while( destx-- )
	{
	    if( srcxinc )
		a = *srcxptr;
	    srcxpos += xscale;
	    srcxinc = srcxpos >> 16;
	    srcxpos &= 0xffff;
	    // D(bug("picture.datatxpe/TC2TC Scale: srcx %d destx %d srcxpos %06lx srcxinc %d\n", srcx, destx, srcxpos, srcxinc));
	    *destxptr++ = a;
	    if( srcxinc )
	    {
		srcxptr += srcxinc;
	    }
	}
    }
    else
    {
	destx = destwidth;
	while( destx-- )
	{
	    if( srcxinc )
	    {
		srcxpixel = srcxptr;
		if( srcpixelbytes == 4 )
		    a = *srcxpixel++;
		r = *srcxpixel++;
		g = *srcxpixel++;
		b = *srcxpixel++;
	    }
	    srcxpos += xscale;
	    srcxinc = srcxpos >> 16;
	    srcxpos &= 0xffff;
	    // D(bug("picture.datatxpe/TC2TC Scale: srcx %d destx %d srcxpos %06lx srcxinc %d\n", srcx, destx, srcxpos, srcxinc));
	    *destxptr++ = a;
	    *destxptr++ = r;
	    *destxptr++ = g;
	    *destxptr++ = b;
	    if( srcxinc )
	    {
		if( srcxinc == 1 )
		    srcxptr += srcpixelbytes;
		else if( srcpixelbytes == 4 )
		    srcxptr += srcxinc<<2;
		else
		    srcxptr += (srcxinc<<2) - srcxinc;
	    }
	}
    }
}

static BOOL ScaleArraySimple( struct Picture_Data *pd, struct RastPort rp )
{
    unsigned int srcy, desty, srcyinc;
    ULONG srcypos, pixelformat, success;
    UWORD srcpixelbytes;
    ULONG destwidth, destwidthbytes;
    UBYTE *srcyptr, *destline;

    destwidth = pd->DestWidth;
    destwidthbytes = MOD16( pd->DestWidth << 2 );
    pixelformat = pd->SrcPixelFormat;
    srcpixelbytes = pd->SrcPixelBytes;
    destline = AllocLineBuffer( destwidth, 1, 4 );
    if( !destline )
	return FALSE;
    srcy = 0;
    srcyinc = 1;
    srcypos = 0;
    srcyptr = pd->SrcBuffer;
    for( desty=0; desty<pd->DestHeight; desty++ )
    {
	if( srcyinc )	// incremented source line after last line scaling ?
	    ScaleLineSimple( srcyptr, destline, destwidth, srcpixelbytes, pd->XScale );
	srcypos += pd->YScale;
	srcyinc = (srcypos >> 16) - srcy;
	// D(bug("picture.datatype/TC2TC Scale: srcy %d desty %d srcypos %06lx srcyinc %d\n", srcy, desty, srcypos, srcyinc));
	if( pixelformat == PBPAFMT_LUT8 )
	{
	    success = WriteLUTPixelArray(   destline,		// src buffer
					    0,			// src x
					    0,			// src y
					    destwidth,		// src mod
					    &rp,		// rastport
					    pd->ColTableXRGB,	// coltable
					    0,			// dest x
					    desty,		// dest y
					    destwidth,		// width
					    1,			// height
					    CTABFMT_XRGB8 );	// coltable format
	}
	else
	{
	    success = WritePixelArray(	destline,		// src buffer
					0,			// src x
					0,			// src y
					destwidth,		// src mod
					&rp,			// rastport
					0,			// dest x
					desty,			// dest y
					destwidth,		// width
					1,			// height
					RECTFMT_ARGB);		// src format
	}
	if( !success ) return FALSE;
	if( srcyinc )
	{
	    if( srcyinc == 1 )	srcyptr += pd->SrcWidthBytes;
	    else		srcyptr += pd->SrcWidthBytes * srcyinc;
	    srcy += srcyinc;
	}
    }
    FreeVec( destline );
    return TRUE;
}

/**************************************************************************************************/

BOOL AllocSrcBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes )
{
    pd->SrcWidthBytes = MOD16( width * pixelbytes);
    pd->SrcBuffer = AllocVec( pd->SrcWidthBytes * height, MEMF_ANY );
    if( !pd->SrcBuffer )
    {
	D(bug("picture.datatype/AllocSrcBuffer: Chunky source buffer allocation failed !\n"));
	return FALSE;
    }
    pd->SrcWidth = width;
    pd->SrcHeight = height;
    pd->SrcPixelFormat = pixelformat;
    pd->SrcPixelBytes = pixelbytes;
    D(bug("picture.datatype/AllocSrcBuffer: Chunky source buffer allocated, %ld bytes\n", (long)(pd->SrcWidthBytes * height)));
    return TRUE;
}

static UBYTE * AllocLineBuffer( long width, long height, int pixelbytes )
{
    long widthbytes;
    UBYTE *buffer;

    widthbytes = MOD16( width * pixelbytes);
    buffer = AllocVec( widthbytes * height, MEMF_ANY );
    if( !buffer )
    {
	D(bug("picture.datatype/AllocLineBuffer: Line buffer allocation failed !\n"));
	return FALSE;
    }
    D(bug("picture.datatype/AllocLineBuffer: Line buffer allocated, %ld bytes\n", (long)(widthbytes * height)));
    return buffer;
}

BOOL AllocDestBM( struct Picture_Data *pd )
{
    pd->DestBM = AllocBitMap( pd->DestWidth,
			      pd->DestHeight,
			      pd->DestDepth,
			      BMF_MINPLANES,
			      (pd->UseFriendBM && pd->DestScreen) ? pd->DestScreen->RastPort.BitMap : NULL );
    if( !pd->DestBM )
    {
	D(bug("picture.datatype/AllocDestBM: DestBitmap allocation failed !\n"));
	return FALSE;;
    }
    D(bug("picture.datatype/AllocDestBM: DestBM allocated: Flags %ld Width %ld Height %ld Depth %ld\n", (long)GetBitMapAttr(pd->DestBM, BMA_FLAGS),
	(long)GetBitMapAttr(pd->DestBM, BMA_WIDTH), (long)GetBitMapAttr(pd->DestBM, BMA_HEIGHT), (long)GetBitMapAttr(pd->DestBM, BMA_DEPTH)));
    return TRUE;
}

void FreeSource( struct Picture_Data *pd )
{
    if( pd->SrcBuffer )
    {
	D(bug("picture.datatype/FreeSource: Freeing SrcBuffer\n"));
	FreeVec( (void *) pd->SrcBuffer );
	pd->SrcBuffer = NULL;
    }
    if( pd->SrcBM && !pd->KeepSrcBM )
    {
	D(bug("picture.datatype/FreeSource: Freeing SrcBitmap\n"));
	FreeBitMap( pd->SrcBM );
	pd->SrcBM = NULL;
    }
}

void FreeDest( struct Picture_Data *pd )
{
    int i;

    if( pd->NumAlloc )
    {
	D(bug("picture.datatype/FreeDest: Freeing %ld pens\n", (long)pd->NumAlloc));
	for(i=0; i<pd->NumAlloc; i++)
	{
	    ReleasePen( pd->DestScreen->ViewPort.ColorMap, pd->ColTable[i] );
	}
	pd->NumAlloc=0;
    }
    
    if( pd->MaskPlane )
    {
	D(bug("picture.datatype/FreeDest: Freeing MaskPlane\n"));
	FreeVec( (void *) pd->MaskPlane );
	pd->MaskPlane = NULL;
    }

    if( pd->DestBM )
    {
	D(bug("picture.datatype/FreeDest: Freeing DestBitmap\n"));
	FreeBitMap( pd->DestBM );
	pd->DestBM = NULL;
    }
}

/**************************************************************************************************/

static void CopyColTable( struct Picture_Data *pd )
{
    int i, j;
    ULONG colR, colG, colB;

    j = 0;
    for( i=0; i<256; i++ )
    {
	colR = pd->DestColRegs[j] = pd->SrcColRegs[j];
	j++;
	colG = pd->DestColRegs[j] = pd->SrcColRegs[j];
	j++;
	colB = pd->DestColRegs[j] = pd->SrcColRegs[j];
	j++;
	pd->ColTableXRGB[i] = ((colR>>8) & 0x00ff0000) | ((colG>>16) & 0x0000ff00) | ((colB>>24) & 0x000000ff);
    }
}

void InitGreyColTable( struct Picture_Data *pd )
{
    int i, cnt;
    ULONG * colregs;

    colregs = pd->SrcColRegs;
    cnt = 0;
    for( i=0; i<256; i++ )
    {
	colregs[cnt++] = i<<24;
	colregs[cnt++] = i<<24;
	colregs[cnt++] = i<<24;
    }
}

void InitRGBColTable( struct Picture_Data *pd )
{
    int i, j, k, cnt;
    ULONG * colregs;
    ULONG Col7, Col3;

    Col7 = 0xFFFFFFFF/7;
    Col3 = 0xFFFFFFFF/3;
    colregs = pd->SrcColRegs;
    cnt = 0;
    for( i=0; i<4; i++ ) /* blue */
    {
	for( j=0; j<8; j++ ) /* red */
	{
	    for( k=0; k<8; k++ ) /* green */
	    {
		colregs[cnt++] = j*Col7;
		colregs[cnt++] = k*Col7;
		colregs[cnt++] = i*Col3;
	    }
	}
    }
}

BOOL ConvertBitmap2Chunky( struct Picture_Data *pd )
{
    struct RastPort SrcRP;
    ULONG y, offset;
    ULONG width, height;
    UBYTE *buffer;

    if( !pd->SrcBM )
	return FALSE;
    D(bug("picture.datatype/Bitmap2Chunky: SrcBM; Flags %ld Width %ld Height %ld Depth %ld\n", (long)GetBitMapAttr(pd->SrcBM, BMA_FLAGS),
	(long)GetBitMapAttr(pd->SrcBM, BMA_WIDTH), (long)GetBitMapAttr(pd->SrcBM, BMA_HEIGHT), (long)GetBitMapAttr(pd->SrcBM, BMA_DEPTH)));
    /* Determine size and allocate Chunky source buffer */
    width = pd->bmhd.bmh_Width;
    height = pd->bmhd.bmh_Height;
    if( !AllocSrcBuffer( pd, width, height, PBPAFMT_LUT8, 1 ) )
	return FALSE;

    /* Copy the source Bitmap into the Chunky source buffer */
    InitRastPort( &SrcRP );
    SrcRP.BitMap = pd->SrcBM;
    offset = 0;
    buffer = pd->SrcBuffer;

#ifdef __AROS__
    for(y=0; y<height; y++)
    {
	/* AROS ReadPixelLine/Array8 does not need a temprp */
	ReadPixelLine8( &SrcRP, 0, y, width, &buffer[offset], NULL );
	offset += pd->SrcWidthBytes;
    }
    DeinitRastPort(&SrcRP);
#else
    D(bug("picture.datatype/Bitmap2Chunky: Slow ReadPixel() conversion\n"));
    {
	ULONG x;
	for(y=0; y<height; y++)
	{
	    for(x=0; x<width; x++)
	    {
		buffer[x + offset] = ReadPixel(&SrcRP, x, y);
	    }
	    offset += pd->SrcWidthBytes;
	}
    }
#endif

    return TRUE;
}

BOOL ConvertChunky2Bitmap( struct Picture_Data *pd )
{
    struct RastPort SrcRP;

    if( !pd->SrcBuffer || pd->TrueColorSrc )
	return FALSE;
    if( !pd->SrcBM )
    {
	/* Allocate source Bitmap */
	pd->SrcBM = AllocBitMap( pd->SrcWidth,
				 pd->SrcHeight,
				 pd->bmhd.bmh_Depth,
				 BMF_STANDARD,
				 NULL );
	if( !pd->SrcBM )
	{
	    D(bug("picture.datatype/Chunky2Bitmap: Bitmap allocation failed !\n"));
	    return FALSE;;
	}
	D(bug("picture.datatype/Chunky2Bitmap: SrcBM allocated; Flags %ld Width %ld Height %ld Depth %ld\n", (long)GetBitMapAttr(pd->SrcBM, BMA_FLAGS),
	    (long)GetBitMapAttr(pd->SrcBM, BMA_WIDTH), (long)GetBitMapAttr(pd->SrcBM, BMA_HEIGHT), (long)GetBitMapAttr(pd->SrcBM, BMA_DEPTH)));
    
	/* Copy the Chunky source buffer to the source Bitmap */
	InitRastPort( &SrcRP );
	SrcRP.BitMap = pd->SrcBM;
	WriteChunkyPixels( &SrcRP, 0, 0, pd->SrcWidth-1, pd->SrcHeight-1, pd->SrcBuffer, pd->SrcWidthBytes );
#ifdef __AROS__
	DeinitRastPort( &SrcRP );
#endif
    }
    return TRUE;
}

BOOL CreateMaskPlane( struct Picture_Data *pd )
{
    if( !pd->SrcBuffer || !pd->DestBM || pd->SrcPixelBytes != 1 || pd->Scale)
    {
	D(bug("picture.datatype/CreateMask: Wrong conditions to create a mask !\n"));
	return FALSE;
    }
    if( !pd->MaskPlane && pd->bmhd.bmh_Masking == mskHasTransparentColor )
    {
	int x, y;
	UBYTE transp = pd->bmhd.bmh_Transparent;
	UBYTE *srcbuf = pd->SrcBuffer;
	int srcwidth = pd->SrcWidth;
	int srcheight = pd->SrcHeight;
	int width16 = MOD16( GetBitMapAttr( pd->DestBM, BMA_WIDTH ) );
	int height = GetBitMapAttr( pd->DestBM, BMA_HEIGHT );
	UBYTE *maskptr;
	int maskwidth = width16 / 8;
	ULONG srcwidthadd = pd->SrcWidthBytes - srcwidth * pd->SrcPixelBytes;

	if( !(maskptr = AllocVec( maskwidth * height, MEMF_ANY )) )
	{
	    D(bug("picture.datatype/CreateMask: Mask allocation failed !\n"));
	    return FALSE;
	}
	pd->MaskPlane = maskptr;
	D(bug("picture.datatype/CreateMask: Mask allocated size %d x %d bytes\n", maskwidth, height));
	
	for(y = 0; y < srcheight; y++)
	{
	    UBYTE *maskx = maskptr;
	    UBYTE mask = 0x80;
	    UBYTE maskbyte = 0x00;

	    for(x = 0; x < srcwidth; x++)
	    {		    
		if( *srcbuf++ != transp )
		    maskbyte |= mask;

		mask >>= 1;
		if( !mask )
		{
		    mask = 0x80;
		    *maskx++ = maskbyte;
		    maskbyte = 0x00;
		}
	    }
	    if( mask != 0x80)
		*maskx = maskbyte;
	    maskptr += maskwidth;
	    srcbuf += srcwidthadd;
	}
    }
    return TRUE;
}

/**************************************************************************************************/

static BOOL RemapTC2CM( struct Picture_Data *pd )
{
    unsigned int DestNumColors;
    int i, j, k;
    int srccnt, destcnt, index;
    ULONG *srccolregs, *destcolregs;
    ULONG Col7, Col3;

    pd->NumSparse = pd->NumColors = 256;
    DestNumColors = 1<<pd->DestDepth;
    if( pd->MaxDitherPens )
	DestNumColors = pd->MaxDitherPens;

    /*
     *  Create color tables: src is (already present) in "natural" order,
     *  dest is sorted by priority using a precalculated table;
     *  "natural" is bits: bbrr.rggg
     */
    Col7 = 0xFFFFFFFF/7;
    Col3 = 0xFFFFFFFF/3;
    srccolregs = pd->SrcColRegs;
    srccnt = 0;
    destcolregs = pd->DestColRegs;
    destcnt = 0;
    for( i=0; i<4; i++ ) /* blue */
    {
	for( j=0; j<8; j++ ) /* red */
	{
	    for( k=0; k<8; k++ ) /* green */
	    {
		index = 3 * defcolmap[destcnt++];
		destcolregs[index++] = srccolregs[srccnt++] = j*Col7;
		destcolregs[index++] = srccolregs[srccnt++] = k*Col7;
		destcolregs[index]   = srccolregs[srccnt++] = i*Col3;
	    }
	}
    }

    /*
     *  Allocate Pens and create sparse table for remapping
     */
    RemapPens( pd, 256, DestNumColors );

    /*
     *  Remap line-by-line truecolor source buffer to destination using sparse table
     */
    {
	struct RastPort DestRP;
	ULONG x, srcy, srcyinc, srcypos;
	ULONG desty;
	UBYTE *srcline, *destline, *thissrc, *thisdest;

	UBYTE *srcbuf = pd->SrcBuffer;
	ULONG srcwidth = pd->SrcWidth;
	ULONG destwidth = pd->DestWidth;
	UBYTE *sparsetable = pd->SparseTable;
	BOOL argb = pd->SrcPixelFormat==PBPAFMT_ARGB;
	BOOL scale = pd->Scale;

	srcline = AllocLineBuffer( MAX(srcwidth, destwidth) * 4, 1, 1 );
	if( !srcline )
	    return FALSE;
	if( scale )
	    destline = AllocLineBuffer( destwidth, 1, 1 );
	else
	    destline = srcline;
	if( !destline )
	    return FALSE;

	InitRastPort( &DestRP );
	DestRP.BitMap = pd->DestBM;
	srcy = 0;
	srcyinc = 1;
	srcypos = 0;
	if( pd->DitherQuality )
	{
	    int rval, gval, bval;
	    long rerr, gerr, berr;
	    UBYTE destindex;
	    ULONG *colregs;
	    int feedback;
	    
	    D(bug("picture.datatype/RemapTC2CM: remapping buffer with dither of %d\n", (int)pd->DitherQuality));
	    feedback = 4 - pd->DitherQuality;
	    destcolregs = pd->DestColRegs;
	    for( desty=0; desty<pd->DestHeight; desty++ )
	    {
		if( srcyinc )	// incremented source line after last line scaling ?
		{
		    if( scale )
		    {
			ScaleLineSimple( srcbuf, srcline, destwidth, pd->SrcPixelBytes, pd->XScale );
			argb = TRUE;
			thissrc = srcline;
		    }
		    else
		    {
			thissrc = srcbuf;
		    }
		    thisdest = destline;
		    rerr = gerr = berr = 0;
		    x = destwidth;
		    while( x-- )
		    {
			if( argb )
			    thissrc++; // skip alpha
			if( feedback )
			{
			    rerr >>= feedback;
			    gerr >>= feedback;
			    berr >>= feedback;
			}
			rerr += (*thissrc++);
			gerr += (*thissrc++);
			berr += (*thissrc++);
			rval = CLIP( rerr );
			gval = CLIP( gerr );
			bval = CLIP( berr );
			index = (rval>>2 & 0x38) | (gval>>5 & 0x07) | (bval & 0xc0);
			destindex = sparsetable[index];
			*thisdest++ = destindex;
			colregs = destcolregs + destindex*3;
			rerr -= (*colregs++)>>24;
			gerr -= (*colregs++)>>24;
			berr -= (*colregs)>>24;
		    }
		}
		if( scale )
		{
		    srcypos += pd->YScale;
		    srcyinc = (srcypos >> 16) - srcy;
		}
		WriteChunkyPixels( &DestRP,
				    0,
				    desty,
				    destwidth-1,
				    desty,
				    destline,
				    destwidth );
		if( srcyinc )
		{
		    if( srcyinc == 1 )	srcbuf += pd->SrcWidthBytes;
		    else		srcbuf += pd->SrcWidthBytes * srcyinc;
		    srcy += srcyinc;
		}
	    }
	}
	else
	{
	    D(bug("picture.datatype/RemapTC2CM: remapping buffer without dithering\n"));
	    for( desty=0; desty<pd->DestHeight; desty++ )
	    {
		if( srcyinc )	// incremented source line after last line scaling ?
		{
		    thissrc = srcbuf;
		    thisdest = srcline;
		    x = srcwidth;
		    while( x-- )
		    {
			if( argb )
			    thissrc++; // skip alpha
			index  = (*thissrc++)>>2 & 0x38; // red
			index |= (*thissrc++)>>5 & 0x07; // green
			index |= (*thissrc++)    & 0xc0; // blue
			
			*thisdest++ = sparsetable[index];
		    }
		    if( scale )
			ScaleLineSimple( srcline, destline, destwidth, 1, pd->XScale );
		}
		if( scale )
		{
		    srcypos += pd->YScale;
		    srcyinc = (srcypos >> 16) - srcy;
		}
		WriteChunkyPixels( &DestRP,
				    0,
				    desty,
				    destwidth-1,
				    desty,
				    destline,
				    destwidth );
		if( srcyinc )
		{
		    if( srcyinc == 1 )	srcbuf += pd->SrcWidthBytes;
		    else		srcbuf += pd->SrcWidthBytes * srcyinc;
		    srcy += srcyinc;
		}
	    }
	}
    #ifdef __AROS__
	DeinitRastPort( &DestRP );
    #endif
	FreeVec( (void *) srcline );
	if( scale )
	    FreeVec( (void *) destline );
    }
    return TRUE;
}

static BOOL RemapCM2CM( struct Picture_Data *pd )
{
    struct HistEntry TheHist[256];
    ULONG width, height;
    int DestNumColors, NumColors;
    int i, j, index;

    width = pd->SrcWidth;
    height = pd->SrcHeight;

    NumColors = pd->NumColors;
    DestNumColors = 1<<pd->DestDepth;
    if( pd->MaxDitherPens )
	DestNumColors = pd->MaxDitherPens;
    if( NumColors < DestNumColors )
	DestNumColors = NumColors;

    memset( pd->DestColRegs, 0xFF, 768*sizeof(ULONG) );	/* initialize GRegs table */
    memset( pd->SparseTable, 0x0, 256 );		/* initialize Sparse table */
    pd->NumSparse = NumColors;

    /*
     *  Fill in histogram with source colors
     */
    index = 0;
    for(i=0; i<NumColors; i++)
    {
	TheHist[i].Count = 0;
	TheHist[i].Red   = pd->SrcColRegs[index++];
	TheHist[i].Green = pd->SrcColRegs[index++];
	TheHist[i].Blue  = pd->SrcColRegs[index++];
    }

    /*
     *  Determine the number of colors in histogram
     */
    { 
	UBYTE *sb = pd->SrcBuffer;
    
	for( i=0; i<height; i++ )
	{
	    for( j=0; j<width; j++ )
	    {
		TheHist[sb[j]].Count++;
	    }
	    sb += pd->SrcWidthBytes;
	}
    }
    
    /*
     *  Remove duplicate colors in histogram
     */
    for( i=0; i<NumColors-1; i++ )
    {
	for( j=i+1; j<NumColors; j++ )
	{
	    if( (TheHist[j].Red   == TheHist[i].Red  ) &&
		(TheHist[j].Green == TheHist[i].Green) &&
		(TheHist[j].Blue  == TheHist[i].Blue ) )
	    {
		TheHist[i].Count += TheHist[j].Count;
		TheHist[j].Count = 0;
	    }
	}
    }

    /*
     *  Sort histogram by most used colors
     */
    qsort( (void *) TheHist, NumColors, sizeof(struct HistEntry), HistSort );

    /*
     *  Copy colors to dest, most used colors first
     */
    index = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	pd->DestColRegs[index++] = TheHist[i].Red;
	pd->DestColRegs[index++] = TheHist[i].Green;
	pd->DestColRegs[index++] = TheHist[i].Blue;
    }

    /*
     *  Allocate Pens in this order and create sparse table for remapping
     */
    RemapPens( pd, NumColors, DestNumColors );
    
    /*
     *  Remap source buffer to dest Bitmap
     */
    D(bug("picture.datatype/RemapCM2CM: remapping buffer to new pens\n"));
    { 
	struct RastPort DestRP;
	ULONG x, srcy, srcyinc, srcypos;
	ULONG desty;
	UBYTE *srcline, *destline, *thissrc, *thisdest;

	UBYTE *srcbuf = pd->SrcBuffer;
	ULONG srcwidth = pd->SrcWidth;
	ULONG destwidth = pd->DestWidth;
	BOOL scale = pd->Scale;
	UBYTE *sparsetable = pd->SparseTable;
 
	srcline = AllocLineBuffer( srcwidth, 1, 1 );
	if( !srcline )
	    return FALSE;
	if( scale )
	    destline = AllocLineBuffer( destwidth, 1, 1 );
	else
	    destline = srcline;
	if( !destline )
	    return FALSE;

	InitRastPort( &DestRP );
	DestRP.BitMap = pd->DestBM;
	srcy = 0;
	srcyinc = 1;
	srcypos = 0;
	for( desty=0; desty<pd->DestHeight; desty++ )
	{
	    if( srcyinc )	// incremented source line after last line scaling ?
	    {
		thissrc = srcbuf;
		thisdest = srcline;
		x = srcwidth;
		while( x-- )
		{
		    *thisdest++ = sparsetable[*thissrc++];
		}
		if( scale )
		    ScaleLineSimple( srcline, destline, destwidth, 1, pd->XScale );
	    }
	    if( scale )
	    {
		srcypos += pd->YScale;
		srcyinc = (srcypos >> 16) - srcy;
	    }
	    WriteChunkyPixels( &DestRP,
				0,
				desty,
				destwidth-1,
				desty,
				destline,
				destwidth );
	    if( srcyinc )
	    {
		if( srcyinc == 1 )	srcbuf += pd->SrcWidthBytes;
		else		srcbuf += pd->SrcWidthBytes * srcyinc;
		srcy += srcyinc;
	    }
	}
#ifdef __AROS__
	DeinitRastPort( &DestRP );
#endif
	FreeVec( (void *) srcline );
	if( scale )
	    FreeVec( (void *) destline );
    }

    return TRUE;
}

static int HistSort( const void *HistEntry1, const void *HistEntry2 )
{
    struct HistEntry *HE1, *HE2;
    
    HE1 = (struct HistEntry *) HistEntry1;
    HE2 = (struct HistEntry *) HistEntry2;
    
    return ((int) (HE2->Count - HE1->Count));
}

static void RemapPens( struct Picture_Data *pd, int NumColors, int DestNumColors )
{
    int i, j, index;
    int pen;

    /*
     *  Obtain pens for DestColRegs (GRegs)
     */
    index = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	pd->ColTable[i] = ObtainBestPen( pd->DestScreen->ViewPort.ColorMap,
				         pd->DestColRegs[index+0],
					 pd->DestColRegs[index+1],
					 pd->DestColRegs[index+2],
				         OBP_Precision, pd->Precision,
				         OBP_FailIfBad, FALSE,
				         TAG_DONE);
	// D(bug("picture.datatype/RemapPens: %d Pen %d: R %d G %d B %d\n",
	//     (int)i, (int)pd->ColTable[i], (int)(pd->DestColRegs[index]>>24), (int)(pd->DestColRegs[index+1]>>24), (int)(pd->DestColRegs[index+2]>>24)));
	index += 3;
	pd->NumAlloc++;
    }
    D(bug("picture.datatype/RemapPens: NumColors: %ld DestNumColors: %ld NumAlloc: %ld Depth: %ld\n",
        (long)pd->NumColors, (long)DestNumColors, (long)pd->NumAlloc, (long)pd->DestDepth));
 
    /*
     *  Get Pen's real colors
     */
    for( i=0; i<DestNumColors; i++ )
    {
	pen = pd->ColTable[i];
	GetRGB32( pd->DestScreen->ViewPort.ColorMap, pen, 1, pd->DestColRegs+pen*3 );
	// D(bug("picture.datatype/RemapPens: %d Pen %d: R %d G %d B %d\n",
	//     i, pen, (int)(pd->DestColRegs[pen*3]>>24), (int)(pd->DestColRegs[pen*3+1]>>24), (int)(pd->DestColRegs[pen*3+2]>>24)));
    }
 
    /*
     *  Determine SparseTable by minimum distance method
     */
    index = 0;
    for( i=0; i<NumColors; i++ )
    {
	ULONG Diff, LastDiff;
	int CRed, GRed, CGreen, GGreen, CBlue, GBlue;
    
	LastDiff=0xFFFFFFFF;
    
	CRed   = pd->SrcColRegs[index++]>>17;
	CGreen = pd->SrcColRegs[index++]>>17;
	CBlue  = pd->SrcColRegs[index++]>>17;
    
	for( j=0; j<DestNumColors; j++ )
	{
	    pen = pd->ColTable[j] * 3;
	    GRed   = pd->DestColRegs[pen++]>>17;
	    GGreen = pd->DestColRegs[pen++]>>17;
	    GBlue  = pd->DestColRegs[pen]>>17;
       
	    Diff = abs(CRed   - GRed  ) +
		   abs(CGreen - GGreen) +
		   abs(CBlue  - GBlue );
       
	    if( Diff <= LastDiff )
	    {
		pd->SparseTable[i] = pd->ColTable[j];
		LastDiff = Diff;
	    }
       
	    if(LastDiff==0)
	    {
		break;
	    }
	}
    }

#if 0    /* optionally display resulting sparse table with color values for debugging */
    {
	int sp;
	D(bug("picture.datatype/RemapPens: sparse table: source col -> dest pen\n"));
	for( i=0; i<NumColors; i++ )
	{
	    sp = pd->SparseTable[i];
	    D(bug("picture.datatype/RemapPens: %d (R %d G %d B %d) -> %d (R %d G %d B %d)\n",
		i, pd->SrcColRegs[i*3]>>24, pd->SrcColRegs[i*3+1]>>24, pd->SrcColRegs[i*3+2]>>24,
		sp, pd->DestColRegs[sp*3]>>24, pd->DestColRegs[sp*3+1]>>24, pd->DestColRegs[sp*3+2]>>24));
	}
    }
#endif
}

/**************************************************************************************************/
