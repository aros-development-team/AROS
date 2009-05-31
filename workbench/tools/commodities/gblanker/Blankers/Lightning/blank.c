/*
 **	SPHINX-O-Lightning
 **	a module for Garshneblanker
 **
 **	Copyright (C) 1993 SPHINX.
 **	Programmed by Raymond Penners.
 */

#include <exec/memory.h>
#include <stdio.h>
#include <math.h>
#include "/includes.h"

UWORD ColorsOn[] = { 0x000, 0xFFF, 0xAAA, 0x444 };
#define PaletteOn( V ) LoadRGB4( V, ColorsOn, 4 )

UWORD ColorsOff[] = { 0, 0, 0, 0 };
#define PaletteOff( V ) LoadRGB4( V, ColorsOff, 4 )

UWORD Width, Height, Forkiness;

#include "Lightning_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 6;
	Prefs[2].po_Level = 4;
	Prefs[4].po_ModeID = getTopScreenMode();
}

VOID DrawLine( struct RastPort *Rast, SHORT x1, SHORT y1, SHORT x2, SHORT y2,
			  SHORT col )
{
	if( x1 < 0 )
	{
		if( x2 < 0 )
			return;
		x1 = 0;
	}
	else if( x1 >= Width )
	{
		if( x2 >= Width )
			return;
		x1 = Width - 1;
	}
	
	if( y1 < 0 )
	{
		if( y2 < 0 )
			return;
		y1 = 0;
	}
	else if( y1 >= Height )
	{
		if( y2 >= Height )
			return;
		y1 = Height - 1;
	}

	if( x2 < 0 )
		x2 = 0;
	else if( x2 >= Width )
		x2 = Width - 1;
	
	if( y2 < 0 )
		y2 = 0;
	else if( y2 >= Height )
		y2 = Height - 1;
	
	SetAPen( Rast, col );
	Move( Rast, x1, y1 );
	Draw( Rast, x2, y2 );
}

VOID DrawLightning( struct RastPort *Rast, LONG X, LONG Y, LONG Segments,
				   double Dir, LONG Fork )
{
	double Sign = PI/8, xSeg, ySeg;
	LONG nX, nY;

	xSeg = ( double )( Width / ( 15 * Fork ));
	ySeg = ( double )( Height / ( 15 * Fork ));

	while( --Segments &&( Y < Height ))
	{
		LONG Angle = RangeRand( 100 );
		double DeltaAngle = Sign * ( double )Angle / 100.0;
		
		Dir = Dir + DeltaAngle;
		Sign = Sign * -1.0;
		
		nX = X + ( LONG )( cos( Dir ) * xSeg );
		nY = Y + ( LONG )( sin( Dir ) * ySeg );

		DrawLine( Rast, X, Y, nX, nY, Fork );

		X = nX;
		Y = nY;

		if(( Fork < 3 )&&( RangeRand( 50 ) < ( Forkiness / Fork )))
			DrawLightning( Rast, X, Y, 5 + RangeRand( 10 ),
						  Dir - 2.0 * DeltaAngle, Fork + 1 );
	}
}

LONG Pause( LONG Ticks )
{
	LONG RetVal = ContinueBlanking();

	while( RetVal == OK && ( Ticks -= 2 ) > 0 )
	{
		Delay( 2 );
		RetVal = ContinueBlanking();
	}

	return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	LONG ToFrontCount = 0, RetVal, Freq, i;
	struct RastPort *Rast;
	struct ViewPort *View;
	struct Screen *Scr;
	struct Window *Wnd;
	
	Scr = OpenScreenTags( 0L, SA_DisplayID, Prefs[4].po_ModeID, SA_Depth, 2,
						 SA_Quiet, TRUE, SA_Behind, TRUE,
						 SA_Overscan, OSCAN_STANDARD, TAG_DONE );
	if( Scr )
	{
		Forkiness = Prefs[0].po_Level;
		Freq = Prefs[2].po_Level;
		
		Rast = &Scr->RastPort;
		View = &Scr->ViewPort;
		Width = Scr->Width;
		Height = Scr->Height;
		
		SetDrMd( Rast, JAM1 );
		Wnd = BlankMousePointer( Scr );
		
		PaletteOff( View );

		do
		{
			Move( Rast, 0, 0 );
			SetRast( Rast, 0 );
			if( RangeRand( 100 ) > 50 )
				DrawLightning( Rast, RangeRand( Width/2 ), 0, 50, PI/4, 1 );
			else
				DrawLightning( Rast, Width/2 + RangeRand( Width/2-1 ), 0,
							  50, 3*PI/4, 1 );
			
			if(!( ToFrontCount++ % 25 ))
				ScreenToFront( Scr );

			i = RangeRand( 100 );
			if( i < 25 )
			{
				int j = 2;
				
				j += i & 1;
				while( j-- > 0 )
				{
					WaitTOF();
					PaletteOn( View );
					Delay( 1 );
					WaitTOF();
					PaletteOff( View );
				}
			}
			else
			{
				PaletteOn( View );
				Delay( 5 );
				WaitTOF();
				PaletteOff( View );
			}
			
			RetVal = Pause( 10 * ( 20 - Freq - RangeRand( 4 )));
		}
		while( RetVal == OK );
		
		UnblankMousePointer( Wnd );
		
		CloseScreen( Scr );
	}
	else
		RetVal = FAILED;
	
	return RetVal;
}
