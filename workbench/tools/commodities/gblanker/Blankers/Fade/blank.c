/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"

#define DELAY   0
#define PERCENT 2
#define TOPSCR  4

#include "Fade_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[DELAY].po_Level = 15;
	Prefs[PERCENT].po_Level = 75;
	Prefs[TOPSCR].po_Active = 0;
}

LONG Blank( PrefObject *Prefs )
{
	struct Screen *FScr;
	LONG RetVal = OK;
	
	if( FScr = cloneTopScreen( FALSE, Prefs[TOPSCR].po_Active ))
	{
		LONG ToFrontCount = 0, PctCount, BPG;
		LONG Delay = Prefs[DELAY].po_Level + 1;
		struct Window *Wnd;
		ULONG *ColorTable;
		
		Wnd = BlankMousePointer( FScr );
		ColorTable = GetColorTable( FScr );
		
		BPG = AvgBitsPerGun( getTopScreenMode());
		PctCount = ( 1L << BPG ) * Prefs[PERCENT].po_Level / 100;
		
		do
		{
			ToFrontCount++;
			WaitTOF();
			
			if( !( ToFrontCount%Delay ) && ( ToFrontCount/Delay ) < PctCount )
				FadeAndLoadTable( FScr, BPG, ColorTable, 0 );
			
			if(!( ToFrontCount % 60 ))
				ScreenToFront( FScr );
			
			if(!( ToFrontCount % 5 ))
				RetVal = ContinueBlanking();
		}
		while( RetVal == OK );
		
		UnblankMousePointer( Wnd );
		CloseScreen( FScr );
	}
	else
		RetVal = FAILED;

	return RetVal;
}
