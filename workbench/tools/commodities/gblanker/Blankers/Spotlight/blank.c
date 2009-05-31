/*
 *	Copyright (c) 1994 Johan Billing <johan.billing@kcc.ct.se>
 *	All rights reserved.
 *
 *	Michael D. Bayne is free to do whatever he wants with this source...
 *
 */

#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include "/includes.h"

#define AREA_SIZE 200
#define NUM_SPOTS ( Prefs[4].po_Level )
#define SPEED ( Prefs[6].po_Level )

#define abs( x ) ( (x) < 0 ? -(x) : (x) )

struct Spot
{
	LONG x, dx;
	LONG y, dy;
};

struct AreaInfo areainfo;
struct TmpRas tmpras;
struct RastPort TmpRP;

#include "Spotlight_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Active = 0;
	Prefs[2].po_Level = 45;
	Prefs[4].po_Level = 4;
	Prefs[6].po_Level = 4;
}

VOID MyFreeBitMap( struct BitMap *Map )
{
	LONG i;

	if( !Map )
		return;
	
	if( GfxBase->lib_Version > 38 )
	{
		FreeBitMap( Map );
		return;
	}
	
	for( i = 0; i < Map->Depth; i++ )
		if( Map->Planes[i] )
			FreeRaster( Map->Planes[i], Map->BytesPerRow * 8, Map->Rows );

	FreeVec( Map );
}

struct BitMap *MyAllocBitMap( LONG Width, LONG Height, LONG Depth, LONG Flags,
							 struct BitMap *FriendMap )
{
	struct BitMap *NewMap;
	
	if( GfxBase->lib_Version > 38 )
		return AllocBitMap( Width, Height, Depth, Flags, FriendMap );
	
	if( NewMap = AllocVec( sizeof( struct BitMap ), MEMF_CLEAR ))
	{
		LONG i, Failure = FALSE;

		NewMap->BytesPerRow = Width/8 + ( Width%8 ? 1 : 0 );
		NewMap->Rows = Height;
		NewMap->Flags = 0L;
		NewMap->Depth = Depth;
		
		for( i = 0; i < Depth; i++ )
		{
			if(!( NewMap->Planes[i] = AllocRaster( Width, Height )))
			{
				Failure = TRUE;
				break;
			}
			if( Flags & BMF_CLEAR )
				BltClear( NewMap->Planes[i],
						 ( NewMap->BytesPerRow << 16 )|( NewMap->Rows ), 3 );
		}

		if( Failure )
		{
			MyFreeBitMap( NewMap );
			NewMap = 0L;
		}
	}
	
	return NewMap;
}

LONG Blank( PrefObject *Prefs )
{
	LONG xsize, ysize, xrad, yrad;
	LONG i, RetVal = OK, Colors, ToFrontCount = 0;
	ULONG *ColorTable, PctCount, BPG;
	UBYTE *areabuf;
	PLANEPTR rasbuf;
	struct BitMap *DblMap, *CircleMap, *NullMap;
	struct Spot *Spots;
	struct Screen *Scr;
	struct Window *Wnd;

	Spots = AllocVec( sizeof( struct Spot ) * NUM_SPOTS, MEMF_CLEAR );

	/* One more plane needed */
	if( Spots &&( Scr = cloneTopScreen( TRUE, Prefs[0].po_Active )))
	{
		xrad = Scr->Width / 8;
		yrad = Scr->Height / 6;
		
		xsize = 2 * ( xrad + 1 ) + 1;
		ysize = 2 * ( yrad + 1 ) + 1;
		
		areabuf = AllocVec( AREA_SIZE, MEMF_CLEAR );
		rasbuf = AllocRaster( Scr->Width, Scr->Height );
		DblMap = MyAllocBitMap( Scr->Width, Scr->Height,
							   Scr->RastPort.BitMap->Depth, BMF_CLEAR,
							   Scr->RastPort.BitMap );
		if( GfxBase->lib_Version > 38 )
			CircleMap = MyAllocBitMap( xsize+1, ysize+1,
									  Scr->RastPort.BitMap->Depth, BMF_CLEAR,
									  DblMap );
		else
			CircleMap = MyAllocBitMap( Scr->Width, Scr->Height,
									  Scr->RastPort.BitMap->Depth, BMF_CLEAR,
									  DblMap );
		NullMap = MyAllocBitMap( xsize+1, ysize+1, Scr->RastPort.BitMap->Depth,
								BMF_CLEAR, DblMap );
		
		if( areabuf && rasbuf && CircleMap && DblMap && NullMap )
		{
			Colors = 1L << Scr->RastPort.BitMap->Depth;
			ColorTable = GetColorTable( Scr );
			
			if( ColorTable )
			{
				/* Copy lowest colours to highest colours */
				for( i = 0; i < 3*Colors/2; ++i )
					ColorTable[i+3*Colors/2+1]=ColorTable[i+1];
			}
			else
			{
				LONG Col;
				
				for( i = 0; i < Colors/2; ++i )
				{
					Col = GetRGB4( Scr->ViewPort.ColorMap, i );
					SetRGB4( &Scr->ViewPort, i + Colors/2,
							( Col & 0x0F00 ) >> 8, ( Col & 0x00F0 ) >> 4,
							( Col & 0x000F ));
				}
			}
			
			BPG = AvgBitsPerGun( getTopScreenMode());
			PctCount = ( 1L << BPG ) * Prefs[2].po_Level / 100;
			for( i = 0; i < PctCount; i++ )
				FadeAndLoadTable( Scr, BPG, ColorTable, 1 );
			
			Wnd = BlankMousePointer( Scr );
			
			for( i = 0; i < NUM_SPOTS; i++ )
			{
				Spots[i].x = RangeRand( Scr->Width - xsize - 10 ) + 5;
				Spots[i].y = RangeRand( Scr->Height - ysize - 10 ) + 5;
				Spots[i].dx = RangeRand( SPEED ) + 1;
				Spots[i].dy = RangeRand( SPEED ) + 1;
				Spots[i].dx *= ( RangeRand( 100 ) > 50 ) ? 1 : -1;
				Spots[i].dy *= ( RangeRand( 100 ) > 50 ) ? 1 : -1;
			}
			
			/* Only change highest plane */
			SetAPen( &Scr->RastPort, Colors/2 );
			SetWrMsk( &Scr->RastPort, Colors/2 );
			
			/* Draw circle */
			InitRastPort( &TmpRP );
			SetWrMsk( &TmpRP, Colors/2 );
			SetAPen( &TmpRP, Colors/2 );
			InitArea( &areainfo, areabuf, AREA_SIZE/5 );
			TmpRP.AreaInfo = &areainfo;
			InitTmpRas( &tmpras, rasbuf, RASSIZE( Scr->Width, Scr->Height ));
			TmpRP.TmpRas = &tmpras;
			TmpRP.BitMap = CircleMap;
			AreaEllipse( &TmpRP, xrad+1, yrad+1, xrad, yrad );
			AreaEnd( &TmpRP );
			
			while( RetVal == OK )
			{
				for( i = 0; i < NUM_SPOTS; i++ )
				{
					BltBitMap( NullMap, 0, 0, DblMap, Spots[i].x, Spots[i].y,
							  xsize, ysize, 0x0C0, Colors/2, 0L );
					Spots[i].x += Spots[i].dx;
					Spots[i].y += Spots[i].dy;
				}
				
				for( i = 0; i < NUM_SPOTS; i++ )
				{
					if( Spots[i].x < 0 )
					{
						Spots[i].x = 0;
						Spots[i].dx = ( RangeRand( SPEED ) + 1 );
					}
					else if( Spots[i].x > Scr->Width - xsize - 1 )
					{
						Spots[i].x = Scr->Width - xsize - 1;
						Spots[i].dx = -( RangeRand( SPEED ) + 1 );
					}
					if( Spots[i].y < 0 )
					{
						Spots[i].y = 0;
						Spots[i].dy = ( RangeRand( SPEED ) + 1 );
					}
					else if( Spots[i].y > Scr->Height - ysize - 1 )
					{
						Spots[i].y = Scr->Height - ysize - 1;
						Spots[i].dy = -( RangeRand( SPEED ) + 1 );
					}
					BltBitMap( CircleMap, 0, 0, DblMap, Spots[i].x, Spots[i].y,
							  xsize, ysize, 0x0E0, Colors/2, 0L );
				}

				BltBitMap( DblMap, 0, 0, Scr->RastPort.BitMap, 0, 0,
						  Scr->Width, Scr->Height, 0x0C0, Colors/2, 0L );

				if(!( ++ToFrontCount % 60 ))
					ScreenToFront( Scr );
				
				RetVal = ContinueBlanking();
				WaitBlit();
				WaitTOF();
			}
			
			UnblankMousePointer( Wnd );
		}
		else
			RetVal = FAILED;
		
		if( NullMap )
			MyFreeBitMap( NullMap );

		if( DblMap )
			MyFreeBitMap( DblMap );
		
		if( CircleMap )
			MyFreeBitMap( CircleMap );
		
		if( areabuf )
			FreeVec( areabuf );
		
		if( rasbuf )
			FreeRaster( rasbuf, Scr->Width, Scr->Height );
	}
	else
		RetVal = FAILED;
	
	if( Spots )
		FreeVec( Spots );

	if( Scr )
		CloseScreen( Scr );
	
	return RetVal;
}
