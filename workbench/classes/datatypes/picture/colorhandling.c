/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

#include "debug.h"
#include "pictureclass.h"
#include "colorhandling.h"

static void FreeDestBuffer( struct Picture_Data *pd );
static void CopySrc2Dest( struct Picture_Data *pd );
static void CopyColTable( struct Picture_Data *pd );
static BOOL ConvertPlanar2Chunky( struct Picture_Data *pd );
static BOOL ConvertChunky2Planar( struct Picture_Data *pd );
static BOOL RemapCM2CM( struct Picture_Data *pd );
static BOOL RemapTC2CM( struct Picture_Data *pd );
static int HistSort( const void *HistEntry1, const void *HistEntry2 );
static void RemapPens( struct Picture_Data *pd, int NumColors, int DestNumColors );

/**************************************************************************************************/

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

/**************************************************************************************************/

void ConvertTC2TC( struct Picture_Data *pd )
{
    FreeDest( pd );
    CopySrc2Dest( pd );
}

void ConvertCM2TC( struct Picture_Data *pd )
{
    BOOL success;

    FreeDest( pd );
    if( !pd->SrcBuffer )
	success = ConvertPlanar2Chunky( pd );
    CopySrc2Dest( pd );
    CopyColTable( pd );
}

void ConvertCM2CM( struct Picture_Data *pd )
{
    BOOL success;

    FreeDest( pd );
    if( !pd->SrcBuffer )
	success = ConvertPlanar2Chunky( pd );

    if( pd->Remap )
    {
	success = RemapCM2CM( pd );
	if( pd->UseBM )
	{
	    success = ConvertChunky2Planar( pd );
	    FreeDestBuffer( pd );
	}
	if( pd->FreeSource )
	    FreeSource( pd );
    }
    else
    {
        D(bug("picture.datatype/ConvertCM2CM: Remapping disabled\n"));
	CopySrc2Dest( pd );
	CopyColTable( pd );
    }
}

void ConvertTC2CM( struct Picture_Data *pd )
{
    BOOL success;

    FreeDest( pd );
    success = RemapTC2CM( pd );
    if( pd->UseBM )
    {
        success = ConvertChunky2Planar( pd );
        FreeDestBuffer( pd );
    }
    if( pd->FreeSource )
	FreeSource( pd );
}

void CreateDestBM( struct Picture_Data *pd )
{
    BOOL success;

    success = ConvertChunky2Planar( pd );
    FreeDestBuffer( pd );
}

/**************************************************************************************************/

BOOL AllocSrcBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes )
{
    pd->SrcWidthBytes = (width * pixelbytes + 15) & ~15; /* multiple of 16 */
    pd->SrcBuffer = AllocVec( pd->SrcWidthBytes * height, MEMF_ANY );
    if( !pd->SrcBuffer )
    {
	D(bug("picture.datatype/AllocSrcBuffer: Chunky buffer allocation failed !\n"));
	return FALSE;
    }
    pd->SrcWidth = width;
    pd->SrcHeight = height;
    pd->SrcPixelFormat = pixelformat;
    pd->SrcPixelBytes = pixelbytes;
    return TRUE;
}

BOOL AllocDestBuffer( struct Picture_Data *pd, long width, long height, ULONG pixelformat, int pixelbytes )
{
    pd->DestWidthBytes = (width * pixelbytes + 15) & ~15; /* multiple of 16 */
    pd->DestBuffer = AllocVec( pd->DestWidthBytes * height, MEMF_ANY );
    if( !pd->DestBuffer )
    {
	D(bug("picture.datatype/AllocDestBuffer: Chunky buffer allocation failed !\n"));
	return FALSE;
    }
    pd->DestWidth = width;
    pd->DestHeight = height;
    pd->DestPixelFormat = pixelformat;
    pd->DestPixelBytes = pixelbytes;
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
	D(bug("picture.datatype/FreeSource: Freeing SrcBM\n"));
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
    
    FreeDestBuffer( pd );

    if( pd->DestBM && pd->DestBM != pd->SrcBM )
    {
	D(bug("picture.datatype/FreeDest: Freeing DestBM\n"));
	FreeBitMap( pd->DestBM );
	pd->DestBM = NULL;
    }
}

static void FreeDestBuffer( struct Picture_Data *pd )
{
    if( pd->DestBuffer && pd->DestBuffer != pd->SrcBuffer )
    {
	D(bug("picture.datatype/FreeDest: Freeing DestBuffer\n"));
	FreeVec( (void *) pd->DestBuffer );
	pd->DestBuffer = NULL;
    }
}

/**************************************************************************************************/

static void CopySrc2Dest( struct Picture_Data *pd )
{
    pd->DestBuffer      = pd->SrcBuffer;
    pd->DestWidth       = pd->SrcWidth;
    pd->DestWidthBytes  = pd->SrcWidthBytes;
    pd->DestHeight      = pd->SrcHeight;
    pd->DestPixelFormat = pd->SrcPixelFormat;
    pd->DestPixelBytes   = pd->SrcPixelBytes;
}

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

static BOOL ConvertPlanar2Chunky( struct Picture_Data *pd )
{
    struct RastPort SrcRP;
    ULONG y, offset;
    ULONG width, height;
    UBYTE *buffer;

    /* Determine size and allocate Chunky source buffer */
    width = pd->bmhd.bmh_Width;
    height = pd->bmhd.bmh_Height;
    if( !AllocSrcBuffer( pd, width, height, PBPAFMT_LUT8, 1 ) )
	return FALSE;

    /* Copy the planar Bitmap into the Chunky source buffer */
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
    D(bug("picture.datatype/Planar2Chunky: Slow ReadPixel() conversion\n"));
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

    D(bug("picture.datatype/Planar2Chunky: Conversion done\n"));
    return TRUE;
}

static BOOL ConvertChunky2Planar( struct Picture_Data *pd )
{
    struct RastPort DestRP;

    /* Allocate destination Bitmap */
    pd->DestBM = AllocBitMap( pd->DestWidth,
			      pd->DestHeight,
			      pd->DestDepth,
			      (BMF_INTERLEAVED | BMF_MINPLANES),
			      pd->DestScreen->RastPort.BitMap );
    if( !pd->DestBM )
    {
	D(bug("picture.datatype/Planar2Chunky: Chunky buffer allocation failed !\n"));
	return FALSE;;
    }

    /* Copy the Chunky destination buffer to the destination Bitmap */
    InitRastPort( &DestRP );
    DestRP.BitMap = pd->DestBM;
    WriteChunkyPixels( &DestRP, 0, 0, pd->DestWidth-1, pd->DestHeight-1, pd->DestBuffer, pd->DestWidthBytes );
#ifdef __AROS__
    DeinitRastPort( &DestRP );
#endif

    D(bug("picture.datatype/Chunky2Planar: Conversion done\n"));
    return TRUE;
}

/**************************************************************************************************/

static BOOL RemapTC2CM( struct Picture_Data *pd )
{
    ULONG width, height;
    ULONG x, y, widthadd;
    unsigned int DestNumColors;
    int i, j, k, srccnt, destcnt, index;
    ULONG *srccolregs, *destcolregs;
    ULONG Col7, Col3;
    UBYTE *srcbuf, *destbuf;
    BOOL argb;
    ULONG srcwidthadd, destwidthadd;

    D(bug("picture.datatype/RemapTC2CM: alloc dest and init pens\n"));
    width = pd->SrcWidth;
    height = pd->SrcHeight;
    if( !AllocDestBuffer( pd, width, height, PBPAFMT_LUT8, 1 ) )
	return FALSE;

    pd->NumSparse = pd->NumColors = 256;
    DestNumColors = 1<<pd->DestDepth;
    if( pd->MaxDitherPens )
	DestNumColors = pd->MaxDitherPens;

    /*
     *  Create color tables, src is in "natural" order, dest is sorted by priority using a precalculated table;
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
     *  Remap truecolor data buffer to chunky buffer using sparse table
     */
    D(bug("picture.datatype/RemapTC2CM: remap buffer\n"));
    argb = pd->SrcPixelFormat==PBPAFMT_ARGB;
    srcbuf = pd->SrcBuffer;
    srcwidthadd = pd->SrcWidthBytes - pd->SrcWidth * pd->SrcPixelBytes;
    destbuf = pd->DestBuffer;
    destwidthadd = pd->DestWidthBytes - pd->DestWidth * pd->DestPixelBytes;
    for( y=0; y<height; y++ )
    {
	for( x=0; x<width; x++ )
	{
	    if( argb )
		srcbuf++; // skip alpha
	    index  = (*srcbuf++)>>2 & 0x38; // red
	    index |= (*srcbuf++)>>5 & 0x07; // green
	    index |= (*srcbuf++)    & 0xc0; // blue
	    
	    *destbuf++ = pd->SparseTable[index];
	}
	srcbuf += srcwidthadd;
	destbuf += destwidthadd;
    }

    D(bug("picture.datatype/RemapTC2CM: done\n"));
    return TRUE;
}

static BOOL RemapCM2CM( struct Picture_Data *pd )
{
    struct HistEntry TheHist[256];
    ULONG width, height;
    int DestNumColors, NumColors;
    int i, j, ic, jc;

    D(bug("picture.datatype/RemapCM2CM: alloc dest and init pens\n"));
    width = pd->SrcWidth;
    height = pd->SrcHeight;
    if( !AllocDestBuffer( pd, width, height, PBPAFMT_LUT8, 1 ) )
	return FALSE;

    NumColors = pd->NumColors;
    DestNumColors = 1<<pd->DestDepth;
    if( pd->MaxDitherPens )
	DestNumColors = pd->MaxDitherPens;
    if( NumColors < DestNumColors )
	DestNumColors = NumColors;

    memset( pd->DestColRegs, 0xFF, 768*sizeof(ULONG) );	/* initialize GRegs table */
    memset( pd->SparseTable, 0x0, 256 );		/* initialize Sparse table */
    pd->NumSparse = NumColors;

    D(bug("picture.datatype/RemapCM2CM: sorting pens\n"));
    /*
     *  Farben im Histogramm ausfuellen
     */
    ic = 0;
    for(i=0; i<NumColors; i++)
    {
	TheHist[i].Count = 0;
	TheHist[i].Red   = pd->SrcColRegs[ic++];
	TheHist[i].Green = pd->SrcColRegs[ic++];
	TheHist[i].Blue  = pd->SrcColRegs[ic++];
    }

    /*
     *  Farbanzahl im Histogramm ermitteln
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
     *  Duplikate im Histogramm ausmerzen
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
     *  Histogramm nach Haeufigkeit sortieren
     */
    qsort( (void *) TheHist, NumColors, sizeof(struct HistEntry), HistSort );

    /*
     *  Es werden die DestNumColors meistvorhandenen Farben benutzt
     */
    ic = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	pd->DestColRegs[ic++] = TheHist[i].Red;
	pd->DestColRegs[ic++] = TheHist[i].Green;
	pd->DestColRegs[ic++] = TheHist[i].Blue;
    }

    /*
     *  Allocate Pens and create sparse table for remapping
     */
    RemapPens( pd, NumColors, DestNumColors );
    
    /*
     *  ChunkyBuffer remappen
     */
    D(bug("picture.datatype/RemapCM2CM: remap chunky buffer\n"));
    { 
	UBYTE *sb = pd->SrcBuffer;
	UBYTE *db = pd->DestBuffer;
 
	// D(bug("picture.datatype/RemapCM2CM: sb %08lx db %08lx\n", sb, db));
	for( i=0; i<height;i++ )
	{
	    for( j=0; j<width; j++ )
	    {
		db[j] = pd->SparseTable[sb[j]];
	    }
	    sb += pd->SrcWidthBytes;
	    db += pd->DestWidthBytes;
	}
	sb = pd->SrcBuffer;
	db = pd->DestBuffer;
	// for( i=0; i<8;i++ )
	//    D(bug("picture.datatype/RemapCM2CM: sb %08lx db %08lx\n", ((ULONG*)sb)[i], ((ULONG*)db)[i]));
    }

    D(bug("picture.datatype/RemapCM2CM: done\n"));
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
    int i, j, ic, jc;

    /*
     *  Pens fuer DestColRegs (GRegs) obtainen
     */
    D(bug("picture.datatype/RemapPens: obtaining pens, precision %d\n", (int)pd->Precision));
    ic = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	pd->ColTable[i] = jc = ObtainBestPen( pd->DestScreen->ViewPort.ColorMap,
				         pd->DestColRegs[ic+0],
					 pd->DestColRegs[ic+1],
					 pd->DestColRegs[ic+2],
				         OBP_Precision, pd->Precision,
				         OBP_FailIfBad, FALSE,
				         TAG_DONE);
	// D(bug("picture.datatype/RemapPens: %d Pen %d: R %d G %d B %d\n",
	//     (int)i, (int)pd->ColTable[i], (int)(pd->DestColRegs[ic]>>24), (int)(pd->DestColRegs[ic+1]>>24), (int)(pd->DestColRegs[ic+2]>>24)));
	ic += 3;
	pd->NumAlloc++;
    }
    D(bug("picture.datatype/RemapPens: NumColors: %ld DestNumColors: %ld NumAlloc: %ld Depth: %ld\n",
	(long)pd->NumColors, (long)DestNumColors, (long)pd->NumAlloc, (long)pd->DestDepth));
 
    /*
     *  Die wirklichen Farben der Pens holen
     */
    D(bug("picture.datatype/RemapPens: get pens' real colors\n"));
    ic = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	GetRGB32( pd->DestScreen->ViewPort.ColorMap, pd->ColTable[i], 1, pd->DestColRegs+ic );
	// D(bug("picture.datatype/RemapPens: %d Pen %d: R %d G %d B %d\n",
	//     (int)i, (int)pd->ColTable[i], (int)(pd->DestColRegs[ic]>>24), (int)(pd->DestColRegs[ic+1]>>24), (int)(pd->DestColRegs[ic+2]>>24)));
	ic += 3;
    }
 
    /*
     *  SparseTable nach der "Geringster Abstand" Methode bestimmen
     */
    D(bug("picture.datatype/RemapPens: determine sparse table\n"));
    ic = 0;
    for( i=0; i<NumColors; i++ )
    {
	unsigned int Diff, LastDiff;
	short CRed, GRed, CGreen, GGreen, CBlue, GBlue;
    
	LastDiff=0xFFFFFFFF;
    
	CRed   = pd->SrcColRegs[ic++]>>17;
	CGreen = pd->SrcColRegs[ic++]>>17;
	CBlue  = pd->SrcColRegs[ic++]>>17;
	// D(bug("picture.datatype/RemapPens:  SrcCol %d: R %d G %d B %d\n", i, CRed>>7, CGreen>>7, CBlue>>7));
    
	jc = 0;
	for( j=0; j<DestNumColors; j++ )
	{
	    GRed   = pd->DestColRegs[jc++]>>17;
	    GGreen = pd->DestColRegs[jc++]>>17;
	    GBlue  = pd->DestColRegs[jc++]>>17;
       
	    Diff = abs(CRed   - GRed  ) +
		   abs(CGreen - GGreen) +
		   abs(CBlue  - GBlue );
	    // D(bug("picture.datatype/RemapPens: DestCol %d: R %d G %d B %d Diff %ld\n", j, GRed>>7, GGreen>>7, GBlue>>7, (long)Diff));
       
	    if( Diff <= LastDiff )
	    {
		pd->SparseTable[i] = pd->ColTable[j];
		// D(bug("picture.datatype/RemapPens: %d -> %d: %d diff %ld\n", (int)i, (int)j, (int)pd->ColTable[j], (long)Diff));
		LastDiff = Diff;
	    }
       
	    if(LastDiff==0)
	    {
		break;
	    }
	}
    }
    // for( i=0; i<NumColors; i++ )
    //     D(bug("picture.datatype/RemapPens: src col %d (R %d G %d B %d) -> dest pen %d\n",
    //         i, pd->SrcColRegs[i*3]>>24, pd->SrcColRegs[i*3+1]>>24, pd->SrcColRegs[i*3+2]>>24, pd->SparseTable[i]));
}

/**************************************************************************************************/

#if 0
unsigned int *MakeARGB(unsigned long *ColRegs, unsigned int Count)
{
 unsigned int *ARGB;
 register unsigned int i;

 ARGB=NULL;

 if(!(ColRegs && Count))
 {
  return(NULL);
 }

 ARGB=AllocVec(Count*sizeof(unsigned int), MEMF_ANY | MEMF_CLEAR);
 if(!ARGB)
 {
  return(NULL);
 }

 for(i=0; i<Count; i++)
 {
  ARGB[i]  = ((*(ColRegs++)) & 0xFF000000) >>  8;
  ARGB[i] |= ((*(ColRegs++)) & 0xFF000000) >> 16;
  ARGB[i] |= ((*(ColRegs++)) & 0xFF000000) >> 24;
 }

 return(ARGB);
}

unsigned int CountColors(unsigned int *ARGB, unsigned int Count)
{
 unsigned int NumColors;
 register unsigned int i, j;

 NumColors=0;

 if(!(ARGB && Count))
 {
  return(0);
 }

 for(i=0; i<Count; i++)
 {
  /*
   *  We assume that it is a new color.
   */

  NumColors++;

  for(j=0; j<i; j++)
  {
   if(ARGB[j]==ARGB[i])
   {
    /*
     *  Oops, it isn't a new color.
     */

    NumColors--;

    break;
   }
  }
 }

 return(NumColors);
}
#endif
