/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <utility/date.h>
#include <hardware/custom.h>

#include <proto/utility.h>

#include <string.h>

#include "../includes.h"

extern __far struct Custom custom;

#define MIL   0
#define SECS  1
#define FONT  3
#define CYCLE 5
#define DELAY 6
#define MODE  8

LONG Hour[] = {12,1,2,3,4,5,6,7,8,9,10,11,12,1,2,3,4,5,6,7,8,9,10,11};

struct Library *UtilityBase;
struct Library *DiskfontBase;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[MIL].po_Active = 1;
	Prefs[SECS].po_Active = 1;
	Prefs[CYCLE].po_Active = 0;
	strcpy( Prefs[FONT].po_Name, "topaz.font" );
	Prefs[FONT].po_Attr.ta_YSize = 11;
	Prefs[DELAY].po_Level = 60;
	Prefs[MODE].po_ModeID = getTopScreenMode();
}

LONG getTime( BYTE *time, PrefObject *Prefs )
{
	struct DateStamp Stamp;
	struct ClockData Data;
	LONG pos = 0, hour;
	
	DateStamp( &Stamp );
	Amiga2Date((( Stamp.ds_Days * 1440 ) + Stamp.ds_Minute ) * 60 +
			   Stamp.ds_Tick/TICKS_PER_SECOND, &Data );
	
	if( !Prefs[MIL].po_Active )
		hour = Hour[Data.hour];
	else
		hour = Data.hour;

	if( hour > 9 )
		time[pos++] = ( BYTE )( hour / 10 ) + '0';
	time[pos++] = ( BYTE )( hour % 10 ) + '0';

	time[pos++] = ':';

	time[pos++] = ( BYTE )( Data.min / 10 ) + '0';
	time[pos++] = ( BYTE )( Data.min % 10 ) + '0';

	if( Prefs[SECS].po_Active )
	{
		time[pos++] = ':';
		time[pos++] = ( BYTE )( Data.sec / 10 ) + '0';
		time[pos++] = ( BYTE )( Data.sec % 10 ) + '0';
	}

	if( !Prefs[MIL].po_Active )
	{
		if( Data.hour > 11 )
			time[pos++] = 'p';
		else
			time[pos++] = 'a';
		time[pos++] = 'm';
	}

	time[pos] = '\0';
	
	return ( LONG )strlen( time );
}

LONG Diff( BYTE *One, BYTE *Two )
{
	while( *One && *Two )
		if( *One++ != *Two++ )
			return TRUE;

	return *One != *Two;
}

LONG Blank( PrefObject *Prefs )
{
	struct TextFont *font;
	struct Screen *Scr;
	struct Window *Wnd;
	struct RastPort *RP;
	struct ViewPort *VP;
	LONG vals[] = {
		1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 14, 13, 12, 11, 10,
		9, 8, 7, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
	BYTE scrClock[32], oldClock[32];
	LONG ScrToFrontCnt=0, c1 = 0, c2 = 14, c3 = 28, delay, fonty, RetVal = OK;
	LONG x, y, Wid, Hei, len, nlen, numc, base, count = 0, dx = -1, dy = -1;
	
	delay = Prefs[DELAY].po_Level;
	
	Prefs[FONT].po_Attr.ta_Name = Prefs[FONT].po_Name;
	font = OpenDiskFont(&( Prefs[FONT].po_Attr ));
	if( !font )
	{
		strcpy( Prefs[FONT].po_Attr.ta_Name, "topaz.font" );
		font = OpenDiskFont(&( Prefs[FONT].po_Attr ));
	}
	base = font->tf_Baseline;
	fonty = Prefs[FONT].po_Attr.ta_YSize;
	
	Scr = OpenScreenTags( 0l, SA_DisplayID, Prefs[MODE].po_ModeID, SA_Depth, 1,
						 SA_Quiet, TRUE, SA_Overscan, OSCAN_STANDARD,
						 SA_Behind, TRUE, SA_Font, &Prefs[FONT].po_Attr,
						 TAG_DONE );
	if( Scr )
	{
		Wid = Scr->Width;
		Hei = Scr->Height;
		RP = &( Scr->RastPort );
		VP = &( Scr->ViewPort );
		
		SetRGB4( VP, 0, 0L, 0L, 0L );
		switch( Prefs[CYCLE].po_Active )
		{
		case 0:
			SetRGB4( VP, 1, vals[c1], vals[c2], vals[c3] );
			break;
		case 1:
			SetRGB4( VP, 1, ( ULONG )RangeRand( 15 ) + 1,
					( ULONG )RangeRand( 15 ) + 1,
					( ULONG )RangeRand( 15 ) + 1 );
			break;
		case 3:
			setCopperList( Hei, 1, VP, &custom );
		case 2:
			SetRGB4( VP, 1, 15, 15, 15 );
			break;
		}
		
		numc = getTime( scrClock, Prefs );
		while(( len = TextLength( RP, scrClock, numc )) >= Wid )
			numc--;
		
		x = ( Wid - len ) / 2;
		y = ( Hei - fonty ) / 2;
		
		SetAPen( RP, 1 );
		Move( RP, x, y + base );
		Text( RP, scrClock, numc );
		
		Wnd = BlankMousePointer( Scr );
		ScreenToFront( Scr );
		
		while( RetVal == OK )
		{
			WaitTOF();

			if(!( ScrToFrontCnt++ % ( 20 * Prefs[SECS].po_Active ? 1 : 6 )))
			{
				numc = getTime( scrClock, Prefs );
				while(( nlen = TextLength( RP, scrClock, numc )) + x > Wid )
					numc--;
				if( Diff( oldClock, scrClock ))
				{
					Move( RP, x, y + base );
					Text( RP, scrClock, numc );
					
					if( nlen < len )
					{
						SetAPen( RP, 0 );
						RectFill( RP, x + nlen, y - 2, x + len + 2,
								 y + fonty + 2 );
						SetAPen( RP, 1 );
					}
					len = nlen;
					strcpy( oldClock, scrClock );
				}
			}
			
			if(!( ScrToFrontCnt % 60 ))
				ScreenToFront( Scr );

			if( !delay || !( ++count % delay ))
			{
				ScrollRaster( RP, dx, dy, x-2, y-2, x+len+2, y+fonty+2 );
				x -= dx;
				y -= dy;
				if( x < 3 )
					dx = -1;
				else
					if( x > Wid-len-3 )
						dx = 1;
				if( y < 3 )
					dy = -1;
				else
					if( y > Hei-fonty-3 )
						dy = 1;
			}

			if( !Prefs[CYCLE].po_Active )
			{
				if(!( count % 10 ))
				{
					c1 = ++c1 % 42;
					c2 = ++c2 % 42;
					c3 = ++c3 % 42;
					SetRGB4( VP, 1, vals[c1], vals[c2], vals[c3] );
				}
			}

			RetVal = ContinueBlanking();
		}
		if( Prefs[CYCLE].po_Active == 3 )
			clearCopperList( VP );
		UnblankMousePointer( Wnd );
		CloseScreen( Scr );
	}
	else
		RetVal = FAILED;
	
	if( font )
		CloseFont( font );
	
 JAIL:
	return RetVal;
}
