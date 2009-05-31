/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include "/includes.h"
#include <stdlib.h>

#include "TicTacToe_rev.h"
STATIC const UBYTE VersTag[] = VERSTAG;

UBYTE *AreaBuf;
PLANEPTR ScratchBuf;
struct AreaInfo InfRec;
struct TmpRas ScratchRast;
struct RastPort TmpRP;

#define PLAYER_X 1
#define PLAYER_O 2

typedef struct _BoardInfo
{
	struct RastPort *bi_Rast;
	LONG bi_Wid;
	LONG bi_Hei;
	LONG bi_Length;
	LONG bi_Width;
	LONG bi_XOff;
	LONG bi_YOff;
	LONG bi_MoveDelay;
	LONG bi_GameDelay;
}
BoardInfo;

LONG Board[9];
BoardInfo BrdInfo;

VOID Defaults( PrefObject *Prefs )
{
	Prefs[0].po_Level = 1;
	Prefs[2].po_Level = 2;
	Prefs[4].po_ModeID = getTopScreenMode();
}

LONG SafeDelay( LONG Ticks )
{
	LONG RetVal = ContinueBlanking();

	while(( RetVal == OK )&&( Ticks > 0 ))
	{
		Delay( 2 );
		RetVal = ContinueBlanking();
		Ticks -= 2;
	}

	return RetVal;
}

VOID OldSloppyLine( BoardInfo *Inf, LONG x1, LONG y1, LONG x2, LONG y2,
				   LONG Case )
{
	struct RastPort *Rast = Inf->bi_Rast;
	LONG w = Inf->bi_Width;

	AreaEllipse( Rast, x1, y1, w/2, w/2 );
	AreaEnd( Rast );

	AreaEllipse( Rast, x2, y2, w/2, w/2 );
	AreaEnd( Rast );
	
	switch( Case )
	{
	case 1:
		AreaMove( Rast, x1 + w/2, y1 );
		AreaDraw( Rast, x2 + w/2, y2 );
		AreaDraw( Rast, x2 - w/2, y2 );
		AreaDraw( Rast, x1 - w/2, y1 );
		break;
	case 2:
		AreaMove( Rast, x1, y1 + w/2 );
		AreaDraw( Rast, x2, y2 + w/2 );
		AreaDraw( Rast, x2, y2 - w/2 );
		AreaDraw( Rast, x1, y1 - w/2 );
		break;
	case 3:
		AreaMove( Rast, x1 + 7*w/20, y1 + 7*w/20 );
		AreaDraw( Rast, x2 + 7*w/20, y2 + 7*w/20 );
		AreaDraw( Rast, x2 - 7*w/20, y2 - 7*w/20 );
		AreaDraw( Rast, x1 - 7*w/20, y1 - 7*w/20 );
		break;
	case 4:
		AreaMove( Rast, x1 - 7*w/20, y1 + 7*w/20 );
		AreaDraw( Rast, x2 - 7*w/20, y2 + 7*w/20 );
		AreaDraw( Rast, x2 + 7*w/20, y2 - 7*w/20 );
		AreaDraw( Rast, x1 + 7*w/20, y1 - 7*w/20 );
		break;
	}

	AreaEnd( Rast );
}

VOID SloppyLine( BoardInfo *Inf, LONG x1, LONG y1, LONG x2, LONG y2 )
{
	LONG dx, dy, x, y, i, w = Inf->bi_Width, Case;
	
	if( x1 == x2 )
		Case = 1;
	else if( y1 == y2 )
		Case = 2;
	else if( y1 > y2 )
		Case = 3;
	else
		Case = 4;

	x1 += ( RangeRand( w ) - w/2 );
	x2 += ( RangeRand( w ) - w/2 );
	y1 += ( RangeRand( w ) - w/2 );
	y2 += ( RangeRand( w ) - w/2 );

	dx = ( x2 - x1 ) / 5;
	dy = ( y2 - y1 ) / 5;

	for( i = 0, x = x1, y = y1; i < 5; x += dx, y += dy, i++ )
	{
		OldSloppyLine( Inf, x, y, x + dx, y + dy, Case );
		Delay( 2 );
	}
}

VOID DrawO( BoardInfo *Inf, LONG Position )
{
	LONG XCenter, YCenter, Radius;
	LONG o1, o2, o3, o4, w = Inf->bi_Width;

	o1 = RangeRand( w/2 ) - w/4;
	o2 = RangeRand( w/2 ) - w/4;
	o3 = RangeRand( w/2 ) - w/4;
	o4 = RangeRand( w/2 ) - w/4;

	Radius = ( Inf->bi_Length/3 - 2*Inf->bi_Width ) / 2 + o3;
	XCenter = Inf->bi_XOff + Inf->bi_Width + Radius +
		(Position%3) * Inf->bi_Length/3 + o1;
	YCenter = Inf->bi_YOff + Inf->bi_Width + Radius +
		(Position/3) * Inf->bi_Length/3 + o2;

	SetAPen( Inf->bi_Rast, 3 );
	
	AreaEllipse( Inf->bi_Rast, XCenter, YCenter, Radius + o3, Radius + o4 );
	Radius -= Inf->bi_Width;
	AreaEllipse( Inf->bi_Rast, XCenter, YCenter, Radius + o3, Radius + o4 );
	AreaEnd( Inf->bi_Rast );
}

VOID DrawX( BoardInfo *Inf, LONG Position )
{
	LONG XOff = Inf->bi_XOff + Inf->bi_Width + (Position%3) * Inf->bi_Length/3;
	LONG YOff = Inf->bi_YOff + Inf->bi_Width + (Position/3) * Inf->bi_Length/3;
	LONG Dimen = Inf->bi_Length/3 - 2*Inf->bi_Width - Inf->bi_Width/2 - 1;
	
	SetAPen( Inf->bi_Rast, 2 );
	SloppyLine( Inf, XOff + Inf->bi_Width/2, YOff + Inf->bi_Width/2,
			   XOff + Dimen, YOff + Dimen );
	SloppyLine( Inf, XOff + Inf->bi_Width/2, YOff + Dimen, XOff + Dimen,
			   YOff + Inf->bi_Width/2 );
}


VOID DrawBoard( BoardInfo *Inf )
{
	SetAPen( Inf->bi_Rast, 1 );
	SloppyLine( Inf, Inf->bi_XOff + Inf->bi_Length/3, Inf->bi_YOff,
			   Inf->bi_XOff + Inf->bi_Length/3,
			   Inf->bi_Hei - Inf->bi_YOff - 1 );
	SloppyLine( Inf, Inf->bi_XOff + 2*Inf->bi_Length/3, Inf->bi_YOff,
			   Inf->bi_XOff + 2*Inf->bi_Length/3,
			   Inf->bi_Hei - Inf->bi_YOff - 1 );
	SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + Inf->bi_Length/3,
			   Inf->bi_Wid - Inf->bi_XOff - 1,
			   Inf->bi_YOff + Inf->bi_Length/3 );
	SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + 2*Inf->bi_Length/3,
			   Inf->bi_Wid - Inf->bi_XOff - 1,
			   Inf->bi_YOff + 2*Inf->bi_Length/3 );
}

LONG CheckForWin( LONG *Board, BoardInfo *Inf )
{
	if( Inf )
		SetAPen( Inf->bi_Rast, 1 );
	
	/* Check horizontal wins */
	if( Board[0] && Board[0] == Board[1] && Board[0] == Board[2] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + Inf->bi_Length/6,
					   Inf->bi_XOff + Inf->bi_Length - 1,
					   Inf->bi_YOff + Inf->bi_Length/6 );
		return Board[0];
	}
	if( Board[3] && Board[3] == Board[4] && Board[3] == Board[5] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + 3*Inf->bi_Length/6,
					   Inf->bi_XOff + Inf->bi_Length - 1,
					   Inf->bi_YOff + 3*Inf->bi_Length/6 );
		return Board[3];
	}
	if( Board[6] && Board[6] == Board[7] && Board[6] == Board[8] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + 5*Inf->bi_Length/6,
					   Inf->bi_XOff + Inf->bi_Length - 1,
					   Inf->bi_YOff + 5*Inf->bi_Length/6 );
		return Board[6];
	}
	
	/* Check vertical wins */
	if( Board[0] && Board[0] == Board[3] && Board[0] == Board[6] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff + Inf->bi_Length/6,
					   Inf->bi_YOff, Inf->bi_XOff + Inf->bi_Length/6,
					   Inf->bi_YOff + Inf->bi_Length - 1 );
		return Board[0];
	}
	if( Board[1] && Board[1] == Board[4] && Board[1] == Board[7] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff + 3*Inf->bi_Length/6,
					   Inf->bi_YOff, Inf->bi_XOff + 3*Inf->bi_Length/6,
					   Inf->bi_YOff + Inf->bi_Length - 1 );
		return Board[1];
	}
	if( Board[2] && Board[2] == Board[5] && Board[2] == Board[8] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff + 5*Inf->bi_Length/6,
					   Inf->bi_YOff, Inf->bi_XOff + 5*Inf->bi_Length/6,
					   Inf->bi_YOff + Inf->bi_Length - 1 );
		return Board[2];
	}
	
	/* Check diagonal wins */
	if( Board[0] && Board[0] == Board[4] && Board[0] == Board[8] )
	{	
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff,
					   Inf->bi_XOff + Inf->bi_Length - 1,
					   Inf->bi_YOff + Inf->bi_Length - 1 );
		return Board[0];
	}
	if( Board[2] && Board[2] == Board[4] && Board[2] == Board[6] )
	{
		if( Inf )
			SloppyLine( Inf, Inf->bi_XOff, Inf->bi_YOff + Inf->bi_Length - 1,
					   Inf->bi_XOff + Inf->bi_Length - 1, Inf->bi_YOff );
		return Board[2];
	}

	return FALSE;
}

LONG Slots( LONG *Board )
{
	LONG i, NumSlots = 9;

	for( i = 0; i < 9; i++ )
		NumSlots -= ( Board[i] ? 1 : 0 );

	return NumSlots;
}

LONG Hypothesise( LONG *Board, LONG Player )
{
	LONG i;

	for( i = 0; i < 9; i++ )
	{
		if( !Board[i] )
		{
			Board[i] = Player;
			if( CheckForWin( Board, 0L ) == Player )
			{
				Board[i] = 0;
				return i;
			}
			Board[i] = 0;
		}
	}

	return -1;
}

LONG PlayGame( BoardInfo *Inf, LONG *Board )
{
	LONG RetVal, i, CurMove, CurPlayer = PLAYER_X, Winner;

	SetRast( Inf->bi_Rast, 0 );
	DrawBoard( Inf );

	for( i = 0; i < 9; i++ )
		Board[i] = 0;

	do
	{
		if(( RetVal = SafeDelay( Inf->bi_MoveDelay )) != OK )
			break;
		
		/* Go for the win. */
		CurMove = Hypothesise( Board, CurPlayer );
		if( CurMove == -1 )
		{
			LONG Other = ( CurPlayer == PLAYER_X ? PLAYER_O : PLAYER_X );

			/* Go for the block. */
			CurMove = Hypothesise( Board, Other );
			if( CurMove == -1 )
			{
				CurMove = RangeRand( Slots( Board )) + 1;
				for( i = 0; i < 9; i++ )
				{
					if( !Board[i] )
						CurMove--;
					if( !CurMove )
					{
						CurMove = i;
						break;
					}
				}
			}
		}
		Board[CurMove] = CurPlayer;
		if( CurPlayer == PLAYER_X )
		{
			DrawX( Inf, CurMove );
			CurPlayer = PLAYER_O;
		}
		else
		{
			DrawO( Inf, CurMove );
			CurPlayer = PLAYER_X;
		}
		Winner = CheckForWin( Board, Inf );
	}
	while( !Winner && Slots( Board ));
	
	if( RetVal == OK )
		return SafeDelay( Inf->bi_GameDelay );
	else
		return RetVal;
}

LONG Blank( PrefObject *Prefs )
{
	LONG Wid, Hei, RetVal;
	struct RastPort *Rast;
	struct ViewPort *View;
	struct Screen *Scr;
	struct Window *Wnd;
	
	Scr = OpenScreenTags( 0L, SA_Depth, 2, SA_Quiet, TRUE,
						 SA_Overscan, OSCAN_STANDARD, SA_Behind, TRUE,
						 SA_DisplayID, Prefs[4].po_ModeID, TAG_DONE );
	if( Scr )
	{
		Wid = Scr->Width;
		Hei = Scr->Height;

		View = &( Scr->ViewPort );
		SetRGB4( View, 0, 0, 0, 0 );
		SetRGB4( View, 1, 0xC, 0xC, 0xC );
		SetRGB4( View, 2, 0xC, 0, 0 );
		SetRGB4( View, 3, 0, 0xC, 0 );
		
		Rast = &( Scr->RastPort );
		SetRast( Rast, 0 );

		AreaBuf = AllocVec( 100, MEMF_CLEAR );
		ScratchBuf = AllocRaster( Wid, Hei );

		if( AreaBuf && ScratchBuf )
		{
			InitArea( &InfRec, AreaBuf, 20 );
			Rast->AreaInfo = &InfRec;
			InitTmpRas( &ScratchRast, ScratchBuf, Wid * Hei );
			Rast->TmpRas = &ScratchRast;
			
			BrdInfo.bi_Rast = Rast;
			BrdInfo.bi_Wid = Wid;
			BrdInfo.bi_Hei = Hei;
			BrdInfo.bi_Length = min( Wid, Hei ) * 9 / 10;
			BrdInfo.bi_XOff = ( Wid - BrdInfo.bi_Length ) / 2;
			BrdInfo.bi_YOff = ( Hei - BrdInfo.bi_Length ) / 2;
			BrdInfo.bi_Width = BrdInfo.bi_Length / 20;
			BrdInfo.bi_MoveDelay = Prefs[0].po_Level * 25;
			BrdInfo.bi_GameDelay = Prefs[2].po_Level * 50;
			
			Wnd = BlankMousePointer( Scr );

			do
			{
				ScreenToFront( Scr );
				RetVal = PlayGame( &BrdInfo, Board );
			}
			while( RetVal == OK );
			
			UnblankMousePointer( Wnd );
		}
		else
			RetVal = FAILED;
		
		if( AreaBuf )
			FreeVec( AreaBuf );
		if( ScratchBuf )
			FreeRaster( ScratchBuf, Wid, Hei );
	}
	else
		RetVal = FAILED;
	
	if( Scr )
		CloseScreen( Scr );
	
	return RetVal;
}
