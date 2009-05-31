/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"
#include <math.h>

#define AREA_SIZE 100
#define GOLDEN_NUMBER (( sqrt( 5.0 ) - 1.0 )/2.0 )
#define GOLDEN_ANGLE ( 2 * PI * ( 1.0 - GOLDEN_NUMBER ))

#include "GoldenSpiral_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 2;
	Prefs[2].po_ModeID = getTopScreenMode();
	Prefs[2].po_Depth = 4;
}

VOID DrawDiamond( struct RastPort *Rast, LONG x, LONG y, LONG r )
{
	AreaMove( Rast, x + r/2, y );
	AreaDraw( Rast, x + r - 1, y + r/2 );
	AreaDraw( Rast, x + r/2, y + r - 1 );
	AreaDraw( Rast, x, y + r/2 );
	AreaEnd( Rast );
}

LONG Spiral( struct Screen *Scr )
{
	double Side = (double)min( Scr->Width, Scr->Height ), Radius, Angle = 0.0;
	LONG Color = 0, x, y, Colors = ( 1L << Scr->RastPort.BitMap->Depth ) - 1;
	LONG Size, RetVal, Iter = 0;
	
	for( Radius = 0.05; Radius < 0.5; Radius = Radius + 0.0001 )
	{
		x = ( LONG )( Radius * sin( Angle ) * Side ) + Scr->Width/2;
		y = ( LONG )( Radius * cos( Angle ) * Side ) + Scr->Height/2;
#ifdef LAME_COLORS		
		Color = ( LONG )((( double )Colors * Radius )) * 2.0;
#endif
		SetAPen( &Scr->RastPort, Color );
		Size = ( LONG )( Side / 50.0 * sqrt( Radius ));
		if( x >= Size/2 && y >= Size/2 && x < Scr->Width-Size/2-1 &&
		   y < Scr->Height-Size/2-1 )
			DrawDiamond( &Scr->RastPort, x-Size/2, y-Size/2, Size );
		Angle = Angle + GOLDEN_ANGLE;
#ifndef LAME_COLORS
		Color = Color % Colors + 1;
#endif
		
		if(!( Iter++ % 10 )&&(( RetVal = ContinueBlanking()) != OK ))
			break;
	}

	return RetVal;
}	

LONG Blank( PrefObject *Prefs )
{
	LONG ToFrontCount = 0, Wid, Hei, ExtendPalette, RetVal = OK;
	struct AreaInfo AreaInf;
	struct TmpRas TmpRast;
	Triplet *ColorTable;
	PLANEPTR RasterBuf;
	struct Screen *Scr;
	struct Window *Wnd;
	STRPTR AreaBuf;
	
	ExtendPalette = Prefs[0].po_Level;
	
	Scr = OpenScreenTags( 0L, SA_Depth, Prefs[2].po_Depth, SA_Quiet, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Behind, TRUE,
						 SA_DisplayID, Prefs[2].po_ModeID, TAG_DONE );
	if( Scr )
	{
		Wid = Scr->Width;
		Hei = Scr->Height;
		
		RasterBuf = AllocRaster( Wid, Hei );
		AreaBuf = AllocVec( AREA_SIZE, MEMF_CLEAR );

		if( RasterBuf && AreaBuf )
		{
			InitArea( &AreaInf, AreaBuf, AREA_SIZE/5 );
			Scr->RastPort.AreaInfo = &AreaInf;
			InitTmpRas( &TmpRast, RasterBuf, RASSIZE( Wid, Hei ));
			Scr->RastPort.TmpRas = &TmpRast;
			
			ColorTable = RainbowPalette( Scr, 0L, 1L, ExtendPalette );
			SetRast( &( Scr->RastPort ), 0 );
			
			Wnd = BlankMousePointer( Scr );
			ScreenToFront( Scr );
			
			RetVal = Spiral( Scr );

			while( RetVal == OK )
			{
				WaitTOF();
				
				if(!( ToFrontCount % ( 10 * ( 5 - ExtendPalette ))))
					RainbowPalette( Scr, ColorTable, 1L, ExtendPalette );
				
#ifdef BLAH_SHMINKE
				if(!( ToFrontCount % 60 ))
					ScreenToFront( Scr );
#endif
				
				if(!( ToFrontCount % 30 ))
					RetVal = ContinueBlanking();
			}

			UnblankMousePointer( Wnd );
			RainbowPalette( 0L, ColorTable, 1L, ExtendPalette );
		}
		else
			RetVal = FAILED;

		if( RasterBuf )
			FreeRaster( RasterBuf, Wid, Hei );

		if( AreaBuf )
			FreeVec( AreaBuf );
	}
	else
		RetVal = FAILED;
	
	if( Scr )
		CloseScreen( Scr );
	
	return RetVal;
}
