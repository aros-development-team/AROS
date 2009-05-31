/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <hardware/custom.h>
#include <string.h>
#include <clib/diskfont_protos.h>
#include <pragmas/diskfont_pragmas.h>
#include "/includes.h"

#define RAND( base, offset ) (( LONG )( RangeRand( base ) + offset ))

#ifndef min
#define min( x, y ) ( (x) < (y) ? (x) : (y) )
#endif

#define STRING 0
#define FONT   2
#define COLORS 4
#define DELAY  6
#define MODE   8

typedef struct _mPoint
{
	LONG x;
	LONG y;
} mPoint;

typedef struct _Line
{
	BYTE ln_Text[128];
	LONG ln_Length;
	LONG ln_Offset;
}
Line;

extern __far struct Custom custom;

LONG vals[] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
	0x0D, 0x0E, 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06,
	0x05, 0x04, 0x03, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	};

VOID Defaults( PrefObject *Prefs )
{
	strcpy( Prefs[STRING].po_Value, "Every good boy deserves fudge." );
	Prefs[FONT].po_Attr.ta_YSize = 75;
	strcpy( Prefs[FONT].po_Name, "topaz.font" );
	Prefs[COLORS].po_Active = 0;
	Prefs[DELAY].po_Level = 60;
	Prefs[MODE].po_ModeID = getTopScreenMode();
}

LONG FillLine( Line *Item, STRPTR String, LONG Pos, struct Screen *Scr )
{
	LONG i, End = 0;
	STRPTR Ptr;

	while( String[Pos] == ' ' )
		Pos++;
	for( i = 0, Ptr = String + Pos;; Ptr++, i++ )
	{
		if( *Ptr == ' ' )
			End = i;
		else if(( *Ptr == '\n' )|| !*Ptr )
		{
			End = i;
			break;
		}
		Item->ln_Text[i] = *Ptr;
		if( TextLength( &Scr->RastPort, Item->ln_Text, i+1 ) >= Scr->Width )
			break;
	}
	if(( *Ptr == ' ' )||( *Ptr == '\n' )|| !End )
		End = i;
	Item->ln_Text[End] = '\0';
	Item->ln_Length = TextLength( &Scr->RastPort, Item->ln_Text,
								 strlen( Item->ln_Text ));
	
	return End + Pos;
}

Line *CalculateExtent( STRPTR String, struct Screen *Scr, LONG *MaxWid,
					  LONG *NumLines )
{
	LONG TxWidth = Scr->RastPort.TxWidth, TxHeight = Scr->RastPort.TxHeight;
	LONG Lines, Length = strlen( String ), LinePos, i;
	Line *LineArray;
	
	Lines = min( Scr->Height / TxHeight, Length * TxWidth / Scr->Width + 2 );
	
	if( LineArray = AllocVec( sizeof( Line ) * Lines, MEMF_CLEAR ))
	{
		LinePos = i = *MaxWid = *NumLines = 0;
		do
		{
			LinePos = FillLine( &LineArray[i], String, LinePos, Scr );
			if( LineArray[i].ln_Length > *MaxWid )
				*MaxWid = LineArray[i].ln_Length;
			*NumLines += 1;
		}
		while( ++i < Lines && LinePos < Length );
		for( i = 0; i < *NumLines; i++ )
			LineArray[i].ln_Offset = ( *MaxWid - LineArray[i].ln_Length ) / 2;
	}

	return LineArray;
}

LONG Blank( PrefObject *Prefs )
{
	LONG Wid, Hei, dx = -1, dy = -1, count = 0, MaxWid, Lns;
	LONG i, c1 = 0, c2 = 14, c3 = 28, RetVal = OK;
	mPoint new, old, size, min, max;
	struct Library *DiskfontBase;
	struct TextFont *font;
	struct Screen *TextScr;
	struct RastPort *Rast;
	struct ViewPort *View;
	struct Window *Wnd;
	Line *Lines = 0L;

	if(!( DiskfontBase = OpenLibrary( "diskfont.library", 37 )))
		return FAILED;
	
	Prefs[FONT].po_Attr.ta_Name = Prefs[FONT].po_Name;
	font = OpenDiskFont( &Prefs[FONT].po_Attr );
	if( !font )
		Prefs[FONT].po_Attr.ta_Name = "topaz.font";
	
	TextScr = OpenScreenTags( 0l, SA_DisplayID, Prefs[MODE].po_ModeID,
							 SA_Depth, 1, SA_Quiet, TRUE, SA_Behind, TRUE,
							 SA_Overscan, OSCAN_STANDARD,
							 SA_Font, &( Prefs[FONT].po_Attr ), TAG_DONE );
	if( !TextScr )
		goto JAIL;

	Rast = &( TextScr->RastPort );
	View = &( TextScr->ViewPort );
	Wid = TextScr->Width;
	Hei = TextScr->Height;
	
	Lines = CalculateExtent( Prefs[STRING].po_Value, TextScr, &MaxWid, &Lns );
	if( !Lines )
		goto JAIL;
	
	SetRGB4( View, 0, 0L, 0L, 0L );
	switch( Prefs[COLORS].po_Active )
	{
	case 0:
		SetRGB4( View, 1, vals[c1], vals[c2], vals[c3] );
		break;
	case 1:
		SetRGB4( View, 1, RAND( 10, 5 ), RAND( 10, 5 ), RAND( 10, 5 ));
		break;
	case 3:
		setCopperList( Hei, 1, View, &custom );
	case 2:
		SetRGB4( View, 1, 15, 15, 15 );
		break;
	}		
	
	size.x = MaxWid + 2;
	size.y = Lns * Prefs[FONT].po_Attr.ta_YSize + 2;
	new.x = old.x = ( Wid - size.x ) / 2;
	new.y = old.y = ( Hei - size.y ) / 2;
	
	min.x = 0;
	min.y = 0;
	max.x = Wid - size.x - 3;
	max.y = Hei - size.y - 3;
	
	SetAPen( Rast, 1 );
	for( i = 0; i < Lns; i++ )
	{
		Move( Rast, old.x + Lines[i].ln_Offset + 1,
			 old.y + i * font->tf_YSize + font->tf_Baseline + 1 );
		Text( Rast, Lines[i].ln_Text, strlen( Lines[i].ln_Text ));
	}
	
	Wnd = BlankMousePointer( TextScr );
	ScreenToFront( TextScr );
	
	while( RetVal == OK )
	{
		WaitTOF();
		
		if(!( count++ % 60 ))
			ScreenToFront( TextScr );
		
		if( !Prefs[COLORS].po_Active && !( count % 10 ))
		{
			c1 = ++c1 % 42;
			c2 = ++c2 % 42;
			c3 = ++c3 % 42;
			SetRGB4( View, 1, vals[c1], vals[c2], vals[c3] );
		}
		
		if( !Prefs[DELAY].po_Level || !( count % Prefs[DELAY].po_Level ))
		{
			new.x += dx;
			new.y += dy;
			
			if( new.x < min.x ) { new.x = min.x; dx = 1; }
			else if( new.x > max.x ) { new.x = max.x; dx = -1; }
			
			if( new.y < min.y ) { new.y = min.y; dy = 1; }
			else if( new.y > max.y ) { new.y = max.y; dy = -1; }
			
			BltBitMapRastPort( Rast->BitMap, old.x, old.y, Rast, new.x,
							  new.y, size.x, size.y, 0xC0 );
			
			old = new;
		}
		RetVal = ContinueBlanking();
	}
	UnblankMousePointer( Wnd );
	
	if( Prefs[COLORS].po_Active == 3 )
		clearCopperList( View );

 JAIL:	
	if( Lines )
		FreeVec( Lines );

	if( TextScr )
		CloseScreen( TextScr );

	if( font )
		CloseFont( font );

	CloseLibrary( DiskfontBase );
	
	return RetVal;
}
