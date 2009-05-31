/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

UWORD Table4[] = {
	0x0000, 0x0B06, 0x0909, 0x060B,
	0x030E, 0x003E, 0x006B, 0x0099,
	0x00B6, 0x00E3, 0x03E0, 0x06B0,
	0x0990, 0x0B60, 0x0E30, 0x0E03
	};

#ifndef min
#define min( x, y ) ( x < y ? x : y )
#endif

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 6;
	Prefs[2].po_ModeID = getTopScreenMode();
}

LONG Blank( PrefObject *Prefs )
{
	LONG i, j, hei, wid, size, stable, side, offx, offy;
	LONG *Phase[2] = { 0L, 0L }, Cur = 0, Pos, Species, Parent;
	LONG Width, Height, RetVal = OK;
	struct Screen *Scr;
	struct Window *Wnd;
	
	CurrentTime(( ULONG * )&offx, ( ULONG * )&offy );

	Scr = OpenScreenTags( 0L, SA_DisplayID, Prefs[2].po_ModeID, SA_Quiet, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Depth, 4, TAG_DONE );
	if( !Scr )
	{
		RetVal = FAILED;
		goto FAIL;
	}
	
	LoadRGB4( &Scr->ViewPort, Table4, 16 );
	Width = Scr->Width;
	Height = Scr->Height;
	hei = Height / Prefs[0].po_Level;
	wid = Width / Prefs[0].po_Level;
	size = wid * hei;
	side = Height / hei;
	offx = ( Width - side * wid ) / 2;
	offy = ( Height - side * hei ) / 2;
	
	Phase[0] = AllocVec( sizeof( LONG ) * size, MEMF_ANY );
	Phase[1] = AllocVec( sizeof( LONG ) * size, MEMF_ANY );
	
	if( !Phase[0] || !Phase[1] )
	{
		RetVal = FAILED;
		goto FAIL;
	}

	Wnd = BlankMousePointer( Scr );
	ScreenToFront( Scr );
		
	while( RetVal == OK )
	{
		for( j = 0; ( j < hei )&&( RetVal == OK ); j++ )
		{
			for( i = 0; i < wid; i++ )
			{
				Pos = j * hei + i;
				Phase[1-Cur][Pos] = Phase[Cur][Pos] = RangeRand( 15 ) + 1;
				SetAPen( &Scr->RastPort, Phase[Cur][Pos] );
				RectFill( &Scr->RastPort, offx + side * i, offy + side * j,
						 offx + side * i + side - 2,
						 offy + side * j + side - 2 );
				if(!( Pos % 200 ))
					if(( RetVal = ContinueBlanking()) != OK )
						break;
			}
		}	

		do
		{
			stable = TRUE;
			CopyMemQuick( Phase[Cur], Phase[1-Cur], size * sizeof( LONG ));
			for( j = 0; ( j < hei )&&( RetVal == OK ); j++ )
			{
				for( i = 0; i < wid; i++ )
				{
					Pos = j * hei + i;
					Species = Phase[Cur][Pos];
					if( Species == 15 )
						Parent = 1;
					else
						Parent = Species + 1;
					if(( Phase[Cur][j*hei+((i+wid-1)%wid)] == Parent )||
					   ( Phase[Cur][j*hei+((i+1)%wid)] == Parent )||
					   ( Phase[Cur][((j+hei-1)%hei)*hei+i] == Parent )||
					   ( Phase[Cur][((j+1)%hei)*hei+i] == Parent ))
					{
						SetAPen( &Scr->RastPort, Phase[1-Cur][Pos] = Parent );
						RectFill( &Scr->RastPort, offx + side * i,
								 offy + side * j, offx + side * i + side - 2,
								 offy + side * j + side - 2 );
						stable = FALSE;
					}
					if(!( Pos % 200 ))
						if(( RetVal = ContinueBlanking()) != OK )
							break;
				}
			}
			Cur = 1 - Cur;
		}
		while( !stable &&( RetVal == OK ));

		for( i = 0; i < 50 && RetVal == OK; i++ )
		{
			Delay( 2 );
			RetVal = ContinueBlanking();
		}
	}
	UnblankMousePointer( Wnd );
	
 FAIL:
	if( Phase[0] )
		FreeVec( Phase[0] );
	if( Phase[1] )
		FreeVec( Phase[1] );	
	if( Scr )
		CloseScreen( Scr );

	return RetVal;
}
