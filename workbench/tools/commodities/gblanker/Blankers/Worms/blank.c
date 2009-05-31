/*
 *	Copyright (c) 1994 Michael D. Bayne.
 *	All rights reserved.
 *
 *	Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <math.h>
#include "/includes.h"

typedef struct _Worm
{
	LONG Current;
	LONG X[25];
	LONG Y[25];
	LONG Length;
	LONG Color;
	double Direction;
} Worm;

#define ExecBase ( *(( struct Library ** )4L ))

LONG BGColor;

#define WORMS   0
#define LENGTH  2
#define SCREEN  4
#define FADEPCT 6

VOID Defaults( PrefObject *Prefs )
{
	Prefs[WORMS].po_Level = 20;
	Prefs[LENGTH].po_Level = 15;
	Prefs[SCREEN].po_Active = 0;
	Prefs[FADEPCT].po_Level = 0;
}

VOID IterateWorm( struct Screen *Scr, Worm *Jim )
{
	LONG x, y, xp, yp;
	double prev;
	struct RastPort *Rast = &Scr->RastPort;

	xp = Jim->X[Jim->Current];
	yp = Jim->Y[Jim->Current];
	prev = Jim->Direction;
	
	Jim->Current = ( Jim->Current + 1 ) % Jim->Length;

	x = Jim->X[Jim->Current];
	y = Jim->Y[Jim->Current];

	SetAPen( Rast, BGColor );
	RectFill( Rast, x-3, y-3, x+2, y+2 );

	Jim->Direction = prev + (( double )RangeRand( 100 ) / 100.0 - 0.5 ) * PID2;
	x = xp + ( LONG )( 5.0 * cos( Jim->Direction ));
	y = yp + ( LONG )( 5.0 * sin( Jim->Direction ));
	if(( x < 4 )||( x > Scr->Width-4 ))
		x = xp;
	if(( y < 4 )||( y > Scr->Height-4 ))
		y = yp;
	
	SetAPen( Rast, Jim->Color );
	RectFill( Rast, x-4, y-4, x+3, y+3 );

	Jim->X[Jim->Current] = x;
	Jim->Y[Jim->Current] = y;
}
				 
LONG Blank( PrefObject *Prefs )
{
	LONG i, j, nw, RetVal = OK, Colors, ToFrontCount = 0;
	struct Screen *Scr;
	struct Window *Wnd;
	Worm *Worms;
	
	nw = Prefs[WORMS].po_Level;

	Scr = cloneTopScreen( FALSE, Prefs[SCREEN].po_Active );
	Worms = AllocVec( sizeof( Worm ) * nw, MEMF_CLEAR );
	
	if( Scr && Worms )
	{
		Colors = 1L << Scr->RastPort.BitMap->Depth;

		if( Prefs[FADEPCT].po_Level )
		{
			ULONG *ColorTable, PctCount, BPG;

			ColorTable = GetColorTable( Scr );
			BPG = AvgBitsPerGun( getTopScreenMode());
			PctCount = ( 1L << BPG ) * Prefs[FADEPCT].po_Level / 100;
			for( i = 0; i < PctCount; i++ )
				FadeAndLoadTable( Scr, BPG, ColorTable, 0 );
		}
		
		if( ExecBase->lib_Version < 39 )
		{
			BGColor = 0;
			SetOPen(&( Scr->RastPort ), BGColor );
		}
		else
		{
			BGColor = FindColor( Scr->ViewPort.ColorMap, 0, 0, 0, -1 );
			SetOutlinePen(&( Scr->RastPort ), BGColor );
		}
		
		for( i = 0; i < nw; i++ )
		{
			Worms[i].Color = RangeRand( Colors - 1 ) + 1;
			if( Worms[i].Color == BGColor )
				Worms[i].Color = ( Worms[i].Color + 1 ) % Colors;
			Worms[i].Length = Prefs[LENGTH].po_Level;
			Worms[i].Direction = ( double )RangeRand( 360 );
			for( j = 0; j < Worms[i].Length; j++ )
			{
				Worms[i].X[j] = Scr->Width/2;
				Worms[i].Y[j] = Scr->Height/2;
			}
		}
		
		Wnd = BlankMousePointer( Scr );
		
		while( RetVal == OK )
		{
			for( i = 0; i < nw; i++ )
				IterateWorm( Scr, &( Worms[i] ));

			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( Scr );
			
			RetVal = ContinueBlanking();
			WaitTOF();
		}
		
		UnblankMousePointer( Wnd );
	}
	else
		RetVal = FAILED;
		
	if( Worms )
		FreeVec( Worms );
	if( Scr )
		CloseScreen( Scr );

	return RetVal;
}
