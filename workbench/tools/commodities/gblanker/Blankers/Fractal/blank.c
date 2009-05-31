/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <math.h>
#include "/includes.h"

#include "Fractal_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_ModeID = getTopScreenMode();
	Prefs[0].po_Depth = 4;
}

LONG FracBlank( struct Screen *Scr, UWORD Wid, UWORD Hei )
{
	float x = 0, y = 0, xx, yy, a, b, c;
	LONG i, xe, ye, flg_end = OK, mod = ( 1L << Scr->BitMap.Depth ) - 1;
	struct RastPort *Rast = &( Scr->RastPort );
	
	SetRast( Rast, 0 );
	ScreenToFront( Scr );
	
	a = (float)RangeRand( 1000 ) / 10 - 50.0;
	do
		b = (float)RangeRand( 1000 ) / 10 - 50.0;
	while( b == 0.0 );
	c = (float)RangeRand( 1000 ) / 10 - 50.0;
	
	for( i = 0; i < 50000 && flg_end == OK; i++ )
	{
		if(!( i % 50 ))
		{
			ScreenToFront( Scr );
			flg_end = ContinueBlanking();
		}
		
		if( x < 0 )
			xx = y + sqrt( fabs( b * x - c ));
		else
			xx = y - sqrt( fabs( b * x - c ));
		yy = a - x;
		xe = Wid / 2 + (SHORT)x;
		ye = Hei / 2 + (SHORT)y;
		
		if(( xe >= 0 )&&( ye >= 0 )&&( xe < Wid )&&( ye < Hei ))
		{
			SetAPen( Rast, ( ReadPixel( Rast, xe, ye ) + 1 ) % mod + 1 );
			WritePixel( Rast, xe, ye );
		}
		
		x = xx;
		y = yy;
	}

	return flg_end;
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *Scr;
	struct Window *Wnd;
	LONG RetVal = OK;
	
	Scr = OpenScreenTags( 0L, SA_DisplayID, Prefs->po_ModeID, SA_Quiet, TRUE,
						 SA_Depth, Prefs->po_Depth, SA_Behind, TRUE,
						 SA_Overscan, OSCAN_STANDARD, TAG_DONE );
	if( Scr )
	{
		Triplet *ColorTable = RainbowPalette( Scr, 0L, 1L, 0L );
		Wnd = BlankMousePointer( Scr );
		
		while( RetVal == OK )
			RetVal = FracBlank( Scr, Scr->Width, Scr->Height );
		
		UnblankMousePointer( Wnd );
		RainbowPalette( 0L, ColorTable, 1L, 0L );
		CloseScreen( Scr );
	}
	else
		RetVal = FAILED;

	return RetVal;
}
