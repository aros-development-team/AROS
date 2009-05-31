/*
 *  Copyright (c) 1994 Michael D. Bayne.
 *  All rights reserved.
 *
 *  Please see the documentation accompanying the distribution for distribution
 *  and disclaimer information.
 */

#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <graphics/videocontrol.h>
#include <intuition/intuitionbase.h>
#include <libraries/reqtools.h>

#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/asl_protos.h>
#include <clib/reqtools_protos.h>
#include <clib/alib_protos.h>

#include <pragmas/exec_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/asl_pragmas.h>
#include <pragmas/reqtools_pragmas.h>

#include "Garshnelib_protos.h"
#include "Garshnelib_rev.h"

STATIC const UBYTE VersTag[] = VERSTAG;

VOID SASM ScreenModeRequest( AREG(0) struct Window *Wnd,
							AREG(1) LONG *Mode, AREG(2) LONG *Depth )
{
	struct AslBase *AslBase;
	
	if( AslBase = ( struct AslBase * )OpenLibrary( AslName, 38L ))
	{
		struct ScreenModeRequester *smRequest;
		
		if( smRequest = AllocAslRequestTags( ASL_ScreenModeRequest, TAG_END ))
		{
			if( Depth )
			{
				if( AslRequestTags( smRequest, ASLSM_Window, Wnd,
								   ASLSM_SleepWindow, TRUE,
								   ASLSM_TitleText, "Screen Mode",
								   ASLSM_InitialDisplayID, *Mode,
								   ASLSM_InitialDisplayDepth, *Depth,
								   ASLSM_DoDepth, TRUE, TAG_END ))
				{
					*Mode = smRequest->sm_DisplayID;
					*Depth = smRequest->sm_DisplayDepth;
				}
			}
			else
			{
				if( AslRequestTags( smRequest, ASLSM_Window, Wnd,
								   ASLSM_SleepWindow, TRUE,
								   ASLSM_TitleText, "Screen Mode",
								   ASLSM_InitialDisplayID, *Mode, TAG_END ))
					*Mode = smRequest->sm_DisplayID;
			}
			FreeAslRequest( smRequest );
		}
		CloseLibrary(( struct Library * )AslBase );
	}
	else
	{
		struct ReqToolsBase *ReqToolsBase;

		if( ReqToolsBase = ( struct ReqToolsBase * )
		   OpenLibrary( REQTOOLSNAME, REQTOOLSVERSION ))
		{
			struct rtScreenModeRequester *smRequest;
			
			if( smRequest = rtAllocRequestA( RT_SCREENMODEREQ, 0L ))
			{
				if( Depth )
				{
					LONG Tags[] = { RT_Window, 0L, RT_LockWindow, TRUE,
										RTSC_Flags, SCREQF_DEPTHGAD, TAG_END };
					LONG ReqTags[] = { RTSC_DisplayID, 0L,
										   RTSC_DisplayDepth, 0L, TAG_END };
					ReqTags[1] = *Mode;
					ReqTags[3] = *Depth;
					rtChangeReqAttrA( smRequest, ( struct TagItem * )ReqTags );
					
					Tags[1] = ( LONG )Wnd;
					if( rtScreenModeRequestA( smRequest, "Screen Mode",
											 ( struct TagItem * )Tags ))
					{
						*Mode = smRequest->DisplayID;
						*Depth = smRequest->DisplayDepth;
					}
				}
				else
				{
					LONG Tags[] = { RT_Window, 0L, RT_LockWindow, TRUE,
										TAG_END };
					LONG ReqTags[] = { RTSC_DisplayID, 0L, TAG_END };
					
					ReqTags[1] = *Mode;
					rtChangeReqAttrA( smRequest, ( struct TagItem * )ReqTags );
					
					Tags[1] = ( LONG )Wnd;
					if( rtScreenModeRequestA( smRequest, "Screen Mode",
											 ( struct TagItem * )Tags ))
						*Mode = smRequest->DisplayID;
				}
				rtFreeRequest( smRequest );
			}
			CloseLibrary(( struct Library * )ReqToolsBase );
		}
	}
}

#define NUMCOLORS 45
LONG spectrum[] = {
	0x0F00, 0x0E10, 0x0D20, 0x0C30, 0x0B40, 0x0A50, 0x0960,	0x0870, 0x0780,
	0x0690, 0x05A0, 0x04B0, 0x03C0, 0x02D0,	0x01E0, 0x00F0, 0x00E1, 0x00D2,
	0x00C3, 0x00B4, 0x00A5,	0x0096, 0x0087, 0x0078, 0x0069, 0x005A, 0x004B,
	0x003C,	0x002D, 0x001E, 0x000F, 0x010E, 0x020D, 0x030C, 0x040B,	0x050A,
	0x0609, 0x0708, 0x0807, 0x0906, 0x0A05, 0x0B04,	0x0C03, 0x0D02, 0x0E01
	};

VOID SASM setCopperList( DREG(0) LONG Hei, DREG(1) LONG Col,
						AREG(0) struct ViewPort *VPort,
						AREG(1) struct Custom *Custom )
{
	struct Library *GfxBase, *IntuitionBase;
	struct UCopList *uCopList;

	IntuitionBase = OpenLibrary( "intuition.library", 37L );
	GfxBase = OpenLibrary( "graphics.library", 37L );
	
	if( IntuitionBase && GfxBase &&
	   ( uCopList = AllocVec( sizeof( struct UCopList ), MEMF_CLEAR )))
	{
		struct TagItem uCopTags[] = {{VTAG_USERCLIP_SET,0L},{VTAG_END_CM,0L}};
		LONG i, spc;

		spc = Hei/NUMCOLORS;
		CINIT( uCopList, NUMCOLORS );
		for( i = 0; i < NUMCOLORS; ++i )
		{
			CWAIT( uCopList, i * spc, 0 );
			CMOVE( uCopList, Custom->color[Col], spectrum[i%NUMCOLORS] );
		}
		CEND( uCopList );
		Forbid();
		VPort->UCopIns = uCopList;
		Permit();
		VideoControl( VPort->ColorMap, uCopTags );
		RethinkDisplay();
	}
	
	if( GfxBase )
		CloseLibrary( GfxBase );
	
	if( IntuitionBase )
		CloseLibrary( IntuitionBase );
}

VOID SASM clearCopperList( AREG(0) struct ViewPort *VPort )
{
	struct Library *GfxBase;
	
	if( GfxBase = OpenLibrary( "graphics.library", 37L ))
	{
		struct TagItem uCopTags[] = {{VTAG_USERCLIP_CLR,0L},{VTAG_END_CM,0L}};
		struct UCopList *uCopList;

		VideoControl( VPort->ColorMap, uCopTags );
		Forbid();
		uCopList = VPort->UCopIns;
		VPort->UCopIns = 0L;
		Permit();
		FreeVec( uCopList );
	}
}

LONG SASM getTopScreenMode( VOID )
{
	struct IntuitionBase *IntuitionBase;
	struct Library *GfxBase;
	struct Screen *pubScr;
	LONG scrMode;
	BPTR lock;
	
	IntuitionBase =
		( struct IntuitionBase * )OpenLibrary( "intuition.library", 37L );
	GfxBase = OpenLibrary( "graphics.library", 37L );
	
	if( IntuitionBase && GfxBase )
	{
		lock = LockIBase( 0 );
		pubScr = IntuitionBase->FirstScreen;
		scrMode = GetVPModeID(&( pubScr->ViewPort));
		UnlockIBase( lock );
	}
	
	if( GfxBase )
		CloseLibrary( GfxBase );
	
	if( IntuitionBase )
		CloseLibrary(( struct Library * )IntuitionBase );
	
	return scrMode;
}

LONG SASM getTopScreenDepth( VOID )
{
	struct IntuitionBase *IntuitionBase;
	LONG Dep = 0;
	
	if( IntuitionBase =
		( struct IntuitionBase * )OpenLibrary( "intuition.library", 37L ))
	{
		struct Screen *PubScr;
		struct DrawInfo *dri;
		BPTR Lock;

		Lock = LockIBase( 0 );
		PubScr = IntuitionBase->FirstScreen;
		if( dri = GetScreenDrawInfo( PubScr ))
		{
			Dep = dri->dri_Depth;
			FreeScreenDrawInfo( PubScr, dri );
		}
		UnlockIBase( Lock );
	
		CloseLibrary(( struct Library * )IntuitionBase );
	}
	
	return Dep;
}

struct Screen *SASM cloneTopScreen( DREG(0) LONG MoreColors,
								   DREG(1) LONG GetPublic )
{
	LONG sMod, sDep, i, Wid, Hei, offx, offy;
	struct IntuitionBase *IntuitionBase;
	struct Screen *Scr, *nScr = 0L;
	struct Rectangle DispRect;
	struct Library *GfxBase;
	struct DrawInfo *dri;
	UWORD *cols;
	BPTR lock;
	
	IntuitionBase =
		( struct IntuitionBase * )OpenLibrary( "intuition.library", 37L );
	GfxBase = OpenLibrary( "graphics.library", 37L );
	
	if( IntuitionBase && GfxBase )
	{
		/* Lock IntuitionBase so nothing goes away */
		lock = LockIBase( 0 );
		
		/* Grab the first screen and get its attributes */
		if( GetPublic )
			Scr = LockPubScreen( 0L );
		else
			Scr = IntuitionBase->FirstScreen;
		sMod = GetVPModeID(&( Scr->ViewPort )); /* Screen Mode ID */
		offx = Scr->LeftEdge; 
		offy = Scr->TopEdge;
		Wid = Scr->Width;
		Hei = Scr->Height;
		if( dri = GetScreenDrawInfo( Scr ))
			sDep = MoreColors ? dri->dri_Depth + 1 : dri->dri_Depth;
		if( cols = AllocVec( sizeof( WORD ) * ( 1L << sDep ), MEMF_CLEAR ))
			for( i = 0; i < ( 1L << dri->dri_Depth ); ++i )
				cols[i] = GetRGB4( Scr->ViewPort.ColorMap, i );
		
		if( GetPublic )
			UnlockPubScreen( 0L, Scr );
		
		UnlockIBase( lock );
		
		QueryOverscan( sMod, &DispRect, OSCAN_TEXT );
		
		Wid = min( Wid, DispRect.MaxX - DispRect.MinX + 1 );
		Hei = min( Hei, DispRect.MaxY - DispRect.MinY + 1 );
		
		if( sMod != INVALID_ID )
		{
			nScr = OpenScreenTags( NULL, SA_DisplayID, sMod, SA_Depth, sDep,
								  SA_Width, Wid, SA_Height, Hei,
								  SA_Behind, TRUE, SA_Quiet, TRUE, TAG_DONE );
			if( nScr )
			{
				BltBitMap( Scr->RastPort.BitMap, offx < 0 ? -offx : 0,
						  offy < 0 ? -offy : 0, nScr->RastPort.BitMap, 0, 0,
						  Wid, Hei, 0x00C0, 0x00FF, NULL );
				LoadRGB4( &(nScr->ViewPort), cols, 1L << sDep );
				if( offx > 0 )
					MoveScreen( nScr, offx, 0 );
				WaitBlit();
				ScreenToFront( nScr );
			}
		}
		
		if( cols )
			FreeVec( cols );
		
		if( dri )
			FreeScreenDrawInfo( Scr, dri );
	}
	
	if( GfxBase )
		CloseLibrary( GfxBase );
	
	if( IntuitionBase )
		CloseLibrary(( struct Library * )IntuitionBase );
	
	return nScr;
}

ULONG *SASM GetColorTable( AREG(0) struct Screen *Screen )
{
	return 0L;
}

LONG SASM AvgBitsPerGun( DREG(0) LONG ModeID )
{
	return 4L;
}

VOID SASM FadeAndLoadTable( AREG(0) struct Screen *Screen, DREG(0) LONG BPG,
						   AREG(1) ULONG *ColorTable, DREG(1) LONG SavePlanes )
{
	struct Library *GfxBase;
	
	if( GfxBase = OpenLibrary( "graphics.library", 37L ))
	{
		LONG NumCols = ( 1L << Screen->RastPort.BitMap->Depth - SavePlanes );
		LONG i, col;
		
		for( i = 0; i < NumCols; ++i )
		{
			col = GetRGB4( Screen->ViewPort.ColorMap, i );
			SetRGB4(&( Screen->ViewPort ), i,
					(( col & 0x0F00 ) >> 8 ) > 0 ?
					(( col & 0x0F00 ) >> 8 ) - 1 : 0,
					(( col & 0x00F0 ) >> 4 ) > 0 ?
					(( col & 0x00F0 ) >> 4 ) - 1 : 0,
					( col & 0x000F ) > 0 ? ( col & 0x000F ) - 1 : 0 );
		}
	}
	
	if( GfxBase )
		CloseLibrary( GfxBase );
}

struct Window *SASM BlankMousePointer( AREG(0) struct Screen *Scr )
{
	struct Library *IntuitionBase, *GfxBase;
	struct Window *Wnd;
	ULONG oldmodes;
	
	IntuitionBase = OpenLibrary( "intuition.library", 37L );
	GfxBase = OpenLibrary( "graphics.library", 37L );
	
	if( IntuitionBase && GfxBase )
	{
		oldmodes = SetPubScreenModes( 0 );
		if( Wnd = OpenWindowTags( 0L, WA_Activate, TRUE, WA_Left, 0, WA_Top, 0,
								 WA_Width, 1, WA_Height, 1, WA_Borderless,
								 TRUE, WA_CustomScreen, Scr, TAG_END ))
		{
			if( Wnd->UserData = AllocVec( 3 * 2 * sizeof( UWORD ),
										 MEMF_CHIP|MEMF_CLEAR ))
			{
				/* Set a pointer sprite of size 16*1 (alloc and clear data for
				   sprite def which is 2 control words, 2 words 4-color data
				   for each line and 2 empty words at the end (as suggested in
				   Hardware Reference Manual)). */
				SetPointer( Wnd, ( UWORD * )Wnd->UserData, 1, 16, 0, 0 );
			}
		}
		SetPubScreenModes( oldmodes );
	}
	
	if( GfxBase )
		CloseLibrary( GfxBase );
	
	if( IntuitionBase )
		CloseLibrary( IntuitionBase );

	return Wnd;
}

VOID SASM UnblankMousePointer( AREG(0) struct Window *Wnd )
{
	struct Library *IntuitionBase;
	
	if( IntuitionBase = OpenLibrary( "intuition.library", 37L ))
	{
		if( Wnd )
		{
			if( Wnd->UserData )
			{
				ClearPointer( Wnd );
				FreeVec( Wnd->UserData );
			}
			CloseWindow( Wnd );
		}
		CloseLibrary( IntuitionBase );
	}
}

Triplet *AllocTable( LONG Colors, LONG Dep, LONG Offset )
{
	return 0L;
}

UWORD Table1[] = { 0, 0x0FFF };
UWORD Table2[] = { 0, 0x0E00, 0x00E0, 0x000E, 0x0E00, 0x00E0, 0x000E };
WORD Table3[] = {
	0,
	0x0E00, 0x0770, 0x00E0, 0x0077, 0x000E, 0x0707, 0x0707, 0x0E00, 0x0770,
	0x00E0, 0x0077, 0x000E, 0x0707, 0x0707 };
UWORD Table4[] = {
	0,
	0x0E03, 0x0B06, 0x0909, 0x060B, 0x030E, 0x003E, 0x006B, 0x0099, 0x00B6,
	0x00E3, 0x03E0, 0x06B0, 0x0990, 0x0B60, 0x0E30, 0x0E03, 0x0B06, 0x0909,
	0x060B, 0x030E, 0x003E, 0x006B, 0x0099, 0x00B6, 0x00E3, 0x03E0, 0x06B0,
	0x0990, 0x0B60, 0x0E30 };
UWORD Table5[] = {
	0,
	0x0F20, 0x0E30, 0x0C50, 0x0B60, 0x0980, 0x0890, 0x06B0, 0x05C0, 0x03E0,
	0x02F0, 0x00F2, 0x00E3, 0x00C5, 0x00B6, 0x0098, 0x006B, 0x005C, 0x003E,
	0x002F, 0x020F, 0x030E, 0x050C, 0x060B, 0x0809, 0x0908, 0x0B06, 0x0C05,
	0x0E03, 0x0F02, 0x0F00, 0x0F00, 0x0F20, 0x0E30, 0x0C50, 0x0B60, 0x0980,
	0x0890, 0x06B0, 0x05C0, 0x03E0, 0x02F0, 0x00F2, 0x00E3, 0x00C5, 0x00B6,
	0x0098, 0x006B, 0x005C, 0x003E, 0x002F, 0x020F, 0x030E, 0x050C, 0x060B,
	0x0809, 0x0908, 0x0B06, 0x0C05, 0x0E03, 0x0F02, 0x0F00, 0x0F00 };
UWORD Table6[] = {
	0,
	0x0F20, 0x0E30, 0x0C50, 0x0B60, 0x0980, 0x0890, 0x06B0, 0x05C0, 0x03E0,
	0x02F0, 0x00F2, 0x00E3, 0x00C5, 0x00B6, 0x0098, 0x006B, 0x005C, 0x003E,
	0x002F, 0x020F, 0x030E, 0x050C, 0x060B, 0x0809, 0x0908, 0x0B06, 0x0C05,
	0x0E03, 0x0F02, 0x0F00, 0x0F00, 0x0F00, 0x0F20, 0x0E30, 0x0C50, 0x0B60,
	0x0980, 0x0890, 0x06B0, 0x05C0, 0x03E0, 0x02F0, 0x00F2, 0x00E3, 0x00C5,
	0x00B6, 0x0098, 0x006B, 0x005C, 0x003E, 0x002F, 0x020F, 0x030E, 0x050C,
	0x060B, 0x0809, 0x0908, 0x0B06, 0x0C05, 0x0E03, 0x0F02, 0x0F00, 0x0F20,
	0x0E30, 0x0C50, 0x0B60, 0x0980, 0x0890, 0x06B0, 0x05C0, 0x03E0, 0x02F0,
	0x00F2, 0x00E3, 0x00C5, 0x00B6, 0x0098, 0x006B, 0x005C, 0x003E, 0x002F,
	0x020F, 0x030E, 0x050C, 0x060B, 0x0809, 0x0908, 0x0B06, 0x0C05, 0x0E03,
	0x0F02, 0x0F00, 0x0F00, 0x0F00, 0x0F20, 0x0E30, 0x0C50, 0x0B60, 0x0980,
	0x0890, 0x06B0, 0x05C0, 0x03E0, 0x02F0, 0x00F2, 0x00E3, 0x00C5, 0x00B6,
	0x0098, 0x006B, 0x005C, 0x003E, 0x002F, 0x020F, 0x030E, 0x050C, 0x060B,
	0x0809, 0x0908, 0x0B06, 0x0C05, 0x0E03, 0x0F02, 0x0F00 };
UWORD *TblPtr[] = { Table1, Table2, Table3, Table4, Table5, Table6 };

Triplet *SASM RainbowPalette( AREG(0) struct Screen *Screen,
							 AREG(1) Triplet *Table, DREG(0) LONG Offset,
							 DREG(1) LONG EP )
{
	struct Library *GfxBase;
	static LONG i = 0L;
	
	if( Screen )
	{
		if( GfxBase = OpenLibrary( "graphics.library", 37L ))
		{
			LONG Colors[] = { 2, 4, 8, 16, 32, 64 };
			LONG Depth = Screen->BitMap.Depth;
			UWORD *Cols, Tmp;
	
			Cols = &( TblPtr[Depth-1][i] );
			Tmp = Cols[0];
			Cols[0] = 0;
			LoadRGB4(&( Screen->ViewPort ), Cols, Colors[Depth-1] );
			Cols[0] = Tmp;
			i = ++i % ( Colors[Depth-1] );
		}
		
		if( GfxBase )
			CloseLibrary( GfxBase );
	}

	return 0L;
}
