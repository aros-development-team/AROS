/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#define STEP_DELAY Prefs[DELAY].po_Level

#define HORIZ   0
#define VERT    2
#define DIVIS   4
#define DELAY   6
#define SCREEN  8
#define FADEPCT 10

typedef struct _mPoint
{
	LONG x;
	LONG y;
} mPoint;

#define UP      0
#define DOWN    1
#define LEFT    2
#define RIGHT   3

#include "Puzzle_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[HORIZ].po_Level = 7;
	Prefs[VERT].po_Level = 5;
	Prefs[DIVIS].po_Level = 5;
	Prefs[DELAY].po_Level = 5;
	Prefs[SCREEN].po_Active = 0;
	Prefs[FADEPCT].po_Level = 10;
}

LONG getNewDir( LONG x, LONG y, LONG xmax, LONG ymax, LONG olddir )
{
	LONG a1[] = { UP, DOWN, LEFT, RIGHT }, dir;
	LONG a2[] = { DOWN, UP, RIGHT, LEFT };

	do
	{
		switch( dir = RangeRand( 4 ))
		{
		case RIGHT: if( !x ) dir = LEFT; break;
		case LEFT: if( x > xmax ) dir = RIGHT; break;
		case DOWN: if( !y ) dir = UP; break;
		case UP: if( y > ymax ) dir = DOWN; break;
		}
	}
	while( a1[dir] == a2[olddir] );
	
	return dir;
}

LONG Blank( PrefObject *Prefs )
{
	LONG i, dir = UP, xmax, ymax, wid, hei, Width, Height, Divs;
	mPoint c = { 0, 0 }, n = { 0, 0 }, t;
	LONG RetVal = OK, ToFrontCount = 0;
	struct Screen *PScr;
	struct Window *Wnd;
	struct RastPort *Rast;
	
	if( PScr = cloneTopScreen( FALSE, Prefs[SCREEN].po_Active ))
	{
		if( Prefs[FADEPCT].po_Level )
		{
			ULONG *ColorTable, PctCount, BPG;

			ColorTable = GetColorTable( PScr );
			BPG = AvgBitsPerGun( getTopScreenMode());
			PctCount = ( 1L << BPG ) * Prefs[FADEPCT].po_Level / 100;
			for( i = 0; i < PctCount; i++ )
				FadeAndLoadTable( PScr, BPG, ColorTable, 0 );
		}
		
		Rast = &( PScr->RastPort );
		Width = PScr->Width;
		Height = PScr->Height;
		if( Prefs[HORIZ].po_Level )
			wid = Width / Prefs[HORIZ].po_Level;
		else
			wid = Width / 3;
		if( Prefs[VERT].po_Level )
			hei = Height / Prefs[VERT].po_Level;
		else
			hei = Height / 3;
		if( Prefs[DIVIS].po_Level )
			Divs = Prefs[DIVIS].po_Level;
		else
			Divs = 4;
		
		SetAPen( Rast, 2 );
		for( i = 0; i < Width; i += wid )
		{
			Move( Rast, i, 0 );
			Draw( Rast, i, Height-1 );
		}
		for( i = 0; i < Height; i += hei )
		{
			Move( Rast, 0, i );
			Draw( Rast, Width-1, i );
		}
		
		SetAPen( Rast, 1 );
		for( i = wid-1; i < Width; i += wid )
		{
			Move( Rast, i, 0 );
			Draw( Rast, i, Height-1 );
		}
		for( i = hei-1; i < Height; i += hei )
		{
			Move( Rast, 0, i );
			Draw( Rast, Width-1, i );
		}
		
		xmax = Width - 2 * wid;
		ymax = Height - 2 * hei;
		
		SetAPen( Rast, 3 );
		
		Wnd = BlankMousePointer( PScr );
		
		while( RetVal == OK )
		{
			LONG step, i;

			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( PScr );
			
			switch( dir = getNewDir( c.x, c.y, xmax, ymax, dir ))
			{
			case UP:
				n.y += hei;
				t = n;
				step = ( n.y - c.y ) / Divs;
				do
				{
					if( step > t.y - c.y )
						step = t.y - c.y;
					t.y -= step;
					for( i = 0; i < STEP_DELAY; i++ )
						WaitTOF();
					BltBitMap( Rast->BitMap, t.x, t.y + step, Rast->BitMap,
							  t.x, t.y, wid, hei, 0xC0, 0xFF, 0L );
					RectFill( Rast, t.x, t.y + hei, t.x + wid - 1,
							 t.y + hei + step - 1 );
					RetVal = ContinueBlanking();
				}
				while( t.y > c.y && RetVal == OK );
				break;
			case DOWN:
				n.y -= hei;
				t = n;
				step = ( c.y - n.y ) / Divs;
				do
				{
					if( step > c.y - t.y )
						step = c.y - t.y;
					t.y += step;
					for( i = 0; i < STEP_DELAY; i++ )
						WaitTOF();
					BltBitMap( Rast->BitMap, t.x, t.y - step, Rast->BitMap,
							  t.x, t.y, wid, hei, 0xC0, 0xFF, 0L );
					RectFill( Rast, t.x, t.y - step, t.x + wid - 1, t.y - 1 );
					RetVal = ContinueBlanking();
				}
				while( t.y < c.y && RetVal == OK );
				break;
			case LEFT:
				n.x += wid;
				t = n;
				step = ( n.x - c.x ) / Divs;
				do
				{
					if( step > t.x - c.x )
						step = t.x - c.x;
					t.x -= step;
					for( i = 0; i < STEP_DELAY; i++ )
						WaitTOF();
					BltBitMap( Rast->BitMap, t.x + step, t.y, Rast->BitMap,
							  t.x, t.y, wid, hei, 0xC0, 0xFF, 0L );
					RectFill( Rast, t.x + wid, t.y, t.x + wid + step - 1,
							 t.y + hei - 1 );
					RetVal = ContinueBlanking();
				}
				while( t.x > c.x && RetVal == OK );
				break;
			case RIGHT:
				n.x -= wid;
				t = n;
				step = ( c.x - n.x ) / Divs;
				do
				{
					if( step > c.x - t.x )
						step = c.x - t.x;
					t.x += step;
					for( i = 0; i < STEP_DELAY; i++ )
						WaitTOF();
					BltBitMap( Rast->BitMap, t.x - step, t.y, Rast->BitMap,
							  t.x, t.y, wid, hei, 0xC0, 0xFF, 0L );
					RectFill( Rast, t.x - step, t.y, t.x - 1, t.y + hei - 1 );
					RetVal = ContinueBlanking();
				}
				while( t.x < c.x && RetVal == OK );
				break;
			}

			c = n;
		}
		
		UnblankMousePointer( Wnd );
		CloseScreen( PScr );
	}
	else
		RetVal = FAILED;
		
	return RetVal;
}
