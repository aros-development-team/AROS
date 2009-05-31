/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

LONG ExtendPal;
Triplet *ColorTable = 0L;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 2;
	Prefs[2].po_ModeID = getTopScreenMode();
	Prefs[2].po_Depth = 4;
}

LONG Interference( struct Screen *Scr )
{
	LONG Wid = Scr->Width, Hei = Scr->Height, RetVal = OK, Pixel = 0, x, y;
	LONG factor, Colors = ( 1L << Scr->BitMap.Depth ) - 1;
	LONG Transition =  Colors * 2 - 1, PixelVal;
	struct RastPort *R = &( Scr->RastPort );

	ScreenToFront( Scr );

	factor = RangeRand( 20 ) + 1;
	for( y = -Hei / 2 + 1; y <= 0 && RetVal == OK; y++ )
	{
		for( x = -Wid / 2 + 1; x <= 0; x++ )
		{
			PixelVal =  (( x * x + y * y ) / factor ) % Transition;
			if( PixelVal >= Colors )
				PixelVal = Transition - PixelVal;
			
			SetAPen( R, PixelVal + 1 );
			WritePixel( R, Wid / 2 - 1 + x, Hei / 2 - 1 + y );
			WritePixel( R, Wid / 2 - 1 - x, Hei / 2 - 1 + y );
			WritePixel( R, Wid / 2 - 1 + x, Hei / 2 - 1 - y );
			WritePixel( R, Wid / 2 - 1 - x, Hei / 2 - 1 - y );
			if(!( ++Pixel % 600 ))
				RetVal = ContinueBlanking();
		}
		RainbowPalette( Scr, ColorTable, 1L, ExtendPal );
		WaitTOF();
	}

	for( x = 0; x < 400 && RetVal == OK; x++ )
	{
		WaitTOF();
		if(!( x % 4 ))
		{
			RainbowPalette( Scr, ColorTable, 1L, ExtendPal );
			RetVal = ContinueBlanking();
		}
	}

	return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *Scr;
	struct Window *Wnd;
	LONG RetVal = OK;
	
	Scr = OpenScreenTags( 0L, SA_Depth, Prefs[2].po_Depth, SA_Quiet, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Behind, TRUE,
						 SA_DisplayID, Prefs[2].po_ModeID, TAG_DONE );
	if( Scr )
	{
		SetRast(&( Scr->RastPort ), 0 );
		ColorTable = RainbowPalette( Scr, 0L, 1L,
									ExtendPal = Prefs[0].po_Level );
		Wnd = BlankMousePointer( Scr );

		ScreenToFront( Scr );

		while( RetVal == OK )
			RetVal = Interference( Scr );

		UnblankMousePointer( Wnd );
		RainbowPalette( NULL, ColorTable, 1L, ExtendPal );
		CloseScreen( Scr );
	}
	else
		RetVal = FAILED;

	return RetVal;
}
