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

/**************************************************************************************************/

void ConvertTC2TC( struct Picture_Data *pd )
{
    FreeDest( pd );
    CopySrc2Dest( pd );
}

void ConvertCM2TC( struct Picture_Data *pd )
{
    FreeDest( pd );
    if( !pd->SrcBuffer )
	ConvertPlanar2Chunky( pd );
    CopySrc2Dest( pd );
    CopyColTable( pd );
}

void ConvertCM2CM( struct Picture_Data *pd )
{
    FreeDest( pd );
    if( !pd->SrcBuffer )
	ConvertPlanar2Chunky( pd );

    if( pd->Remap )
    {
	RemapCM2CM( pd );
	if( pd->UseBM )
	{
	    ConvertChunky2Planar( pd );
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
    FreeDest( pd );
    RemapTC2CM( pd );
    if( pd->UseBM )
    {
        ConvertChunky2Planar( pd );
        FreeDestBuffer( pd );
    }
    if( pd->FreeSource )
	FreeSource( pd );
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
	FreeVec( (void *) pd->SrcBuffer );
	pd->SrcBuffer = NULL;
    }

    if( pd->SrcBM && !pd->KeepSrcBM )
    {
	FreeBitMap( pd->SrcBM );
	pd->SrcBM = NULL;
    }
}

void FreeDest( struct Picture_Data *pd )
{
    int i;

    if( pd->NumAlloc )
    {
	for(i=0; i<pd->NumAlloc; i++)
	{
	    ReleasePen( pd->DestScreen->ViewPort.ColorMap, pd->ColTable[i] );
	}
	pd->NumAlloc=0;
    }
    
    FreeDestBuffer( pd );

    if( pd->DestBM && pd->DestBM != pd->SrcBM )
    {
	FreeBitMap( pd->DestBM );
	pd->DestBM = NULL;
    }
}

static void FreeDestBuffer( struct Picture_Data *pd )
{
    if( pd->DestBuffer && pd->DestBuffer != pd->SrcBuffer )
    {
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
    ULONG width, widthbytes, height;
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
#ifdef __AROS__
    for(y=0; y<height; y++)
    {
	/* AROS ReadPixelLine/Array8 does not need a temprp */
	ReadPixelLine8( &SrcRP, 0, y, width, &buffer[offset], NULL );
	offset += widthbytes;
    }
    DeinitRastPort(&SrcRP);
#else
    {
	ULONG x;
	for(y=0; y<height; y++)
	{
	    for(x=0; x<width; x++)
	    {
		buffer[x + offset] = ReadPixel(&SrcRP, x, y);
	    }
	    offset += widthbytes;
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
    return TRUE;
}

static BOOL RemapCM2CM( struct Picture_Data *pd )
{
    struct HistEntry TheHist[256];
    ULONG width, height;
    unsigned int DestNumColors, NumColors;
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
     *  Pens fuer GRegs obtainen
     */
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
	ic += 3;
	pd->NumAlloc++;
    }
    D(bug("picture.datatype/RemapCM2CM: NumColors: %ld DestNumColors: %ld NumAlloc: %ld Depth: %ld\n",
	(long)pd->NumColors, (long)DestNumColors, (long)pd->NumAlloc, (long)pd->DestDepth));
 
    /*
     *  Die wirklichen Farben der Pens holen
     */
    ic = 0;
    for( i=0; i<DestNumColors; i++ )
    {
	GetRGB32( pd->DestScreen->ViewPort.ColorMap, pd->ColTable[i], 1, pd->DestColRegs+ic );
//	D(bug("picture.datatype/RemapCM2CM: %d Pen: %d R %d G %d B %d\n",
//	    (int)i, (int)pd->ColTable[i], (int)(pd->DestColRegs[ic]>>24), (int)(pd->DestColRegs[ic+1]>>24), (int)(pd->DestColRegs[ic+2]>>24)));
	ic += 3;
    }
 
    /*
     *  SparseTable nach der "Geringster Abstand" Methode bestimmen
     */
    ic = 0;
    for( i=0; i<NumColors; i++ )
    {
	unsigned int Diff, LastDiff;
	short CRed, GRed, CGreen, GGreen, CBlue, GBlue;
    
	LastDiff=0xFFFFFFFF;
    
	CRed   = pd->SrcColRegs[ic++]>>17;
	CGreen = pd->SrcColRegs[ic++]>>17;
	CBlue  = pd->SrcColRegs[ic++]>>17;
    
	jc = 0;
	for( j=0; j<DestNumColors; j++ )
	{
	    GRed   = pd->DestColRegs[jc++]>>17;
	    GGreen = pd->DestColRegs[jc++]>>17;
	    GBlue  = pd->DestColRegs[jc++]>>17;
       
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
 
//    for( i=0; i<NumColors; i++ )
//	D(bug("picture.datatype/RemapCM2CM: sparse %d\n", (int)pd->SparseTable[i]));
    
    /*
     *  ChunkyBuffer remappen
     */
     
    { 
	UBYTE *sb = pd->SrcBuffer;
	UBYTE *db = pd->DestBuffer;
 
	D(bug("picture.datatype/RemapCM2CM: sb %08lx db %08lx\n", sb, db));
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
	for( i=0; i<8;i++ )
	    D(bug("picture.datatype/RemapCM2CM: sb %08lx db %08lx\n", ((ULONG*)sb)[i], ((ULONG*)db)[i]));
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
