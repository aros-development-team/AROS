/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#define DIMEN 0
#define EXTEN 2
#define DIVIS 4
#define MODE  6

#define MAXITER 16

LONG Range[MAXITER], Hei, Wid, Minor, Offx, Offy, ExtendPal, Divisions;
Triplet *ColorTable = 0L;
BYTE Cl, Dimen;

#include "Plasma_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

#define Val( x, y ) ( ReadPixel( &Scr->RastPort, (x)+Offx, (y)+Offy))
#define max( x, y ) ( (x) > (y) ? (x) : (y) )

VOID Defaults( PrefObject *Prefs )
{
	Prefs[DIMEN].po_Active = 0;
	Prefs[EXTEN].po_Level = 2;
	Prefs[DIVIS].po_Active = 2;
	Prefs[MODE].po_ModeID = getTopScreenMode();
	Prefs[MODE].po_Depth = 4;
}

void SetPixel( struct RastPort *R, LONG i, LONG x, LONG y, LONG v )
{
	if(( x < Wid )&&( y < Hei ))
	{
		SetAPen( R, ( v + Cl + RangeRand( Range[i] ) - Range[i]/2 ) % Cl + 1 );
		WritePixel( R, x + Offx, y + Offy );
	}
}

LONG GroovyRound( LONG Val )
{
	LONG i;

	for( i = -1; Val; Val >>= 1, i++ );

	return 1 << i;
}

LONG PlasmaRect( struct Screen *Scr, LONG i, LONG Size, LONG Step, LONG sx,
				LONG sy )
{
	LONG RetVal = OK, z = 0, x, y, nx, ny, hx, hy, MaxX, MaxY;
	struct RastPort *R = &Scr->RastPort;
	
	MaxX = min( sx + Size, Minor );
	MaxY = min( sy + Size, Minor );
	
	for( y = sy; ( y < MaxY )&&( RetVal == OK ); y += Step )
	{
		ny = ( y + Step ) % Minor;
		hy = ( y + Step/2 ) % Minor;
		for( x = sx; ( x < MaxX )&&( RetVal == OK ); x += Step )
		{
			nx = ( x + Step ) % Minor;
			hx = ( x + Step/2 ) % Minor;
			if(( x + Step/2 ) < Minor )
				SetPixel( R, i, hx, y, ( Val( x, y ) + Val( nx, y )) / 2 );
			if(( y + Step/2 ) < Minor )
				SetPixel( R, i, x, hy, ( Val( x, y ) + Val( x, ny )) / 2 );
			if(( x + Step/2 ) < Minor &&( y + Step/2 ) < Minor )
				SetPixel( R, i, hx, hy, ( Val( x, y ) + Val( nx, y ) +
										 Val( x, ny ) + Val( nx, ny ))/4 );
			if(!( ++z % 300 ))
				RetVal = ContinueBlanking();
		}
	}
	
	return ( RetVal == OK )? ContinueBlanking() : RetVal;
}

LONG DrawPlasma( struct Screen *Scr )
{
	LONG RetVal = OK, x, y, i, Step, Side, MaxCells = 0, yCells;
	
	Side = GroovyRound( Minor ) >> Divisions;
	for( yCells = 0, y = 0; ( y < Minor )&&( RetVal == OK ); y += Side )
	{
		yCells++;
		for( x = 0; x < Minor; x += Side )
		{
			SetPixel( &Scr->RastPort, 0, x, y, 0 );
			MaxCells++;
		}
	}

	if( Divisions )
	{
		for( i = 1, Step = Side; ( RetVal == OK )&&( Step > 1 );
			Step >>= 1, i++ )
		{
			LONG *Drawn, Cells = MaxCells;
		
			if( Drawn = AllocVec( sizeof( LONG ) * Cells, MEMF_CLEAR ))
			{
				do
				{
					LONG j = -1, Pick = RangeRand( Cells ) + 1;
					
					while( Pick ) !Drawn[++j] && --Pick;
					Drawn[j] = TRUE;
					RetVal = PlasmaRect( Scr, i, Side, Step,
										( j / yCells ) * Side,
										( j % yCells ) * Side );
				}	
				while(( RetVal == OK )&& --Cells );
				FreeVec( Drawn );
			}
		}
	}
	else
		for( i = 0, Step = Side; ( RetVal == OK )&&( Step > 1 ); Step >>= 1 )
			RetVal = PlasmaRect( Scr, ++i, Minor, Step, 0, 0 );
	
	return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *Scr;
	struct Window *Wnd;
	LONG i, RetVal = OK;
	
	Scr = OpenScreenTags( NULL, SA_Depth, Prefs[MODE].po_Depth, SA_Quiet, TRUE,
						 SA_DisplayID, Prefs[MODE].po_ModeID, SA_Behind, TRUE,
						 SA_Overscan, OSCAN_STANDARD, TAG_DONE );
	if( Scr )
	{
		Cl = 1L << Prefs[MODE].po_Depth - 1;
		Wid = Scr->Width;
		Hei = Scr->Height;
		if( Prefs[DIMEN].po_Active )
		{
			Minor = Hei;
			if( Wid > Hei )
				Offx = ( Wid - Hei )/ 2;
			else
				Offx = 0L;
			Offy = 0L;
		}
		else
		{
			Minor = Wid;
			Offx = 0L;
			if( Hei > Wid )
				Offy = ( Hei - Wid )/ 2;
			else
				Offy = 0;
		}
		for( i = 0; i < MAXITER; i++ )
			Range[i] = 1L << max( Prefs[MODE].po_Depth - i, 1 );
		ExtendPal = Prefs[EXTEN].po_Level;
		Divisions = Prefs[DIVIS].po_Active;
		ColorTable = RainbowPalette( Scr, 0L, 1, ExtendPal );
		Wnd = BlankMousePointer( Scr );
		
		while( RetVal == OK )
		{
			ScreenToFront( Scr );
			SetRast( &Scr->RastPort, 0L );
			RetVal = DrawPlasma( Scr );
			for( i = 0; ( i < Wid * Hei / 100 )&&( RetVal == OK ); i++ )
			{
				WaitTOF();
				if(!( i % 8 ))
				{
					RainbowPalette( Scr, ColorTable, 1, ExtendPal );
					RetVal = ContinueBlanking();
				}
			}
		}
		RainbowPalette( NULL, ColorTable, 1, ExtendPal );
		UnblankMousePointer( Wnd );
		CloseScreen( Scr );
	}
	else
		RetVal = FAILED;
	
	return RetVal;
}
