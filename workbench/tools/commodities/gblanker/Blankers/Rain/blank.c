/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#ifndef max
#define max( x, y ) ( x > y ? x : y )
#endif

#include "Rain_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 10;
	Prefs[2].po_ModeID = getTopScreenMode();
	Prefs[2].po_Depth = 2;
}

LONG Blank( PrefObject *Prefs )
{
	LONG ToFrontCount = 0, Wid, Hei, Drops, x, y, r, i, incr, RetVal = OK;
	struct RastPort *Rast;
	struct Screen *Scr;
	struct Window *Wnd;
	
	Drops = Prefs[0].po_Level;
	
	Scr = OpenScreenTags( 0L, SA_Depth, Prefs[2].po_Depth, SA_Quiet, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Behind, TRUE,
						 SA_DisplayID, Prefs[2].po_ModeID, TAG_DONE );
	if( Scr )
	{
		Wid = Scr->Width;
		Hei = Scr->Height;
		
		Rast = &( Scr->RastPort );
		SetRast( Rast, 0 );
		
		for( i = 0; i < 4; i++ )
			SetRGB4(&( Scr->ViewPort ), i, 4 * i, 4 * i, 4 * i );
		
		Wnd = BlankMousePointer( Scr );
		ScreenToFront( Scr );
		
		while( RetVal == OK )
		{
			if(!( ++ToFrontCount % 60 ))
				ScreenToFront( Scr );
			
			if(!( ToFrontCount % Drops ))
				SetRast(&( Scr->RastPort ), 0 );
			
			r = RangeRand( Wid/13 ) + Wid/25;
			x = RangeRand( Wid - 2*r ) + r;
			y = RangeRand( Hei - 2*r ) + r;
			
			incr = max( Wid/160, 1 );

			for( i = 0; i < r; i += incr )
			{
				WaitTOF();
				SetAPen( &Scr->RastPort,
						( ULONG )RangeRand(( 1L << Prefs[2].po_Depth ) - 1 )
						+ 1 );
				DrawEllipse(&( Scr->RastPort ), x, y, i, i );
				if( i )
				{
					SetAPen(&( Scr->RastPort ), 0 );
					DrawEllipse(&( Scr->RastPort ), x, y, i - incr, i - incr );
				}
			}
			
			RetVal = ContinueBlanking();
		}
		UnblankMousePointer( Wnd );
	}
	else
		RetVal = FAILED;
	
	if( Scr )
		CloseScreen( Scr );
	
	return RetVal;
}
