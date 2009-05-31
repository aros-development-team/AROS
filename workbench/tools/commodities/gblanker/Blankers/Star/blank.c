/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

typedef struct _Coord3D
{
	LONG x;
	LONG y;
	LONG z;
	LONG speed;
}
Coord3D;

typedef struct _Coord2D
{
	LONG x;
	LONG y;
}
Coord2D;

Coord3D *Coords;
Coord2D *OldCoords;

#include "Star_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 250;
	Prefs[2].po_Level = 100;
	Prefs[4].po_ModeID = getTopScreenMode();
}

VOID InitStar( Coord3D *p, LONG speed )
{
	p->x = 1500 - ( LONG )RangeRand( 3000 );
	p->y = 1000 - ( LONG )RangeRand( 2000 );
	p->z = 1000 + ( LONG )RangeRand( 2000 );
	p->speed = ( LONG )RangeRand( 10 ) + speed;
}

LONG Blank( PrefObject *Prefs )
{
	LONG ToFrontCount = 0, Wid, Hei, i, x, y, Stars, Speed, RetVal = OK;
	struct RastPort *Rast;
	struct Screen *Scr;
	struct Window *Wnd;
	Coord3D *p;
	Coord2D *p2;
	
	Stars = Prefs[0].po_Level;
	Speed = Prefs[2].po_Level;
	
	Coords = AllocVec( Stars * sizeof( Coord3D ), MEMF_CLEAR );
	OldCoords = AllocVec( Stars * sizeof( Coord2D ), MEMF_CLEAR );
	Scr = OpenScreenTags( 0L, SA_Depth, 2, SA_Overscan, OSCAN_STANDARD,
						 SA_DisplayID, Prefs[4].po_ModeID, SA_Behind, TRUE,
						 SA_Quiet, TRUE, TAG_DONE );

	if( Coords && OldCoords && Scr )
	{
		Wid = Scr->Width;
		Hei = Scr->Height;
		
		Rast = &( Scr->RastPort );
		SetRast( Rast, 0 );
		for( i = 0; i < 4; i++ )
			SetRGB4(&( Scr->ViewPort ), i, 4 * i, 4 * i, 4 * i );
		
		for( i = 0; i < Stars; i++ )
		{
			InitStar( &Coords[i], Speed );
			OldCoords[i].x = 0;
			OldCoords[i].y = 0;
		}
		
		Wnd = BlankMousePointer( Scr );
		ScreenToFront( Scr );
		
		while( RetVal == OK )
		{
			WaitTOF();

			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( Scr );
			
			for( p2 = OldCoords, p = Coords, i = 0;
				i < Stars; i++, p++, p2++ )
			{
				x = p2->x;
				y = p2->y;
				
				SetAPen( Rast, 0 );
				switch( p->z / 200 )
				{
				case 0:
					WritePixel( Rast, x, y+1 );
					WritePixel( Rast, x+1, y+1 );
				case 1:
				case 2:
				case 3:
				case 4:
					WritePixel( Rast, x+1, y );
				default:
					WritePixel( Rast, x, y );
				}

				p->z -= p->speed;
				if( p->z <= -1000 )
					InitStar( p, Speed );
				
				x = Wid/2 + ( 200 * p->x ) / ( p->z + 1000 );
				y = Hei/2 + ( 200 * p->y ) / ( p->z + 1000 );
				
				if(( x < 0 )||( x > Wid-2 )||( y < 0 )||( y > Hei-2 ))
				{
					InitStar( p, Speed );
					p2->x = 0;
					p2->y = 0;
				}
				else
				{
					p2->x = x;
					p2->y = y;
					SetAPen( Rast, 3 );
					/* Warning: This is a little twisted. */
					switch( p->z/200 )
					{
					case 9:
					case 8:
						SetAPen( Rast, 2 );
						break;
					case 0:
						WritePixel( Rast, x, y+1 );
						WritePixel( Rast, x+1, y+1 );
					case 4:
					case 3:
					case 2:
					case 1:
						WritePixel( Rast, x+1, y );
					case 7:
					case 6:
					case 5:
						break;
					default:
						SetAPen( Rast, 1 );
						break;
					}
					WritePixel( Rast, x, y );
				}
			}
			if(!( ToFrontCount % 5 ))
				RetVal = ContinueBlanking();
		}
		UnblankMousePointer( Wnd );
	}
	else
		RetVal = FAILED;

	if( Scr )
		CloseScreen( Scr );
	if( Coords )
		FreeVec( Coords );
	if( OldCoords )
		FreeVec( OldCoords );

	return RetVal;
}
